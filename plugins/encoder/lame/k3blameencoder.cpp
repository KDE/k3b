/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config.h>

#include "k3blameencoder.h"

#include <k3bcore.h>
#include <k3bpluginfactory.h>

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kinstance.h>
#include <knuminput.h>
#include <kcombobox.h>
#include <kdialogbase.h>

#include <qlayout.h>
#include <qcstring.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qgroupbox.h>
#include <qbuttongroup.h>
#include <qtextcodec.h>
#include <qfile.h>
#include <qslider.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <stdio.h>
#include <lame/lame.h>


K_EXPORT_COMPONENT_FACTORY( libk3blameencoder, K3bPluginFactory<K3bLameEncoder>( "libk3blameencoder" ) )


static const int s_lame_bitrates[] = {
  32,
  40,
  48,
  56,
  64,
  80,
  96,
  112,
  128,
  160,
  192,
  224,
  256,
  320,
  0 // just used for the loops below
};


static const int s_lame_presets[] = {
  56, // ABR for Voice, Radio, Mono, etc.
  90, //

  V6, // ~115 kbps
  V5, // ~130 kbps  | Portable - small size
  V4, // ~160 kbps

  V3, // ~175 kbps
  V2, // ~190 kbps  | HiFi - for home or quite listening
  V1, // ~210 kbps  |
  V0, // ~230 kbps

  320 // ABR 320 neary lossless for archiving (not recommended, use flac instead)
};


static const char* s_lame_preset_strings[] = {
  I18N_NOOP("Low quality (56 kbps)"),
  I18N_NOOP("Low quality (90 kbps)"),

  I18N_NOOP("Portable (average 115 kbps)"),
  I18N_NOOP("Portable (average 130 kbps)"),
  I18N_NOOP("Portable (average 160 kbps)"),

  I18N_NOOP("HiFi (average 175 kbps)"),
  I18N_NOOP("HiFi (average 190 kbps)"),
  I18N_NOOP("HiFi (average 210 kbps)"),
  I18N_NOOP("HiFi (average 230 kbps)"),

  I18N_NOOP("Archiving (320 kbps)"),
};


static const char* s_lame_mode_strings[] = {
  I18N_NOOP("Stereo"),
  I18N_NOOP("Joint Stereo"),
  I18N_NOOP("Mono")
};



class K3bLameEncoder::Private
{
public:
  Private()
    : flags(0),
      fid(0) {
  }

  lame_global_flags* flags;

  char buffer[8000];

  QString filename;
  FILE* fid;
};




K3bLameEncoder::K3bLameEncoder( QObject* parent, const char* name )
  : K3bAudioEncoder( parent, name )
{
  d = new Private();
}


K3bLameEncoder::~K3bLameEncoder()
{
  closeFile();

  delete d;
}


bool K3bLameEncoder::openFile( const QString& extension, const QString& filename, const K3b::Msf& length )
{
  closeFile();

  d->filename = filename;
  d->fid = ::fopen( QFile::encodeName( filename ), "w+" );
  if( d->fid )
    return initEncoder( extension, length );
  else
    return false;
}


bool K3bLameEncoder::isOpen() const
{
  return ( d->fid != 0 );
}


void K3bLameEncoder::closeFile()
{
  if( isOpen() ) {
    finishEncoder();
    ::fclose( d->fid );
    d->fid = 0;
    d->filename.truncate(0);
  }
}


const QString& K3bLameEncoder::filename() const
{
  return d->filename;
}


bool K3bLameEncoder::initEncoderInternal( const QString&, const K3b::Msf& length )
{
  KConfig* c = k3bcore->config();
  c->setGroup( "K3bLameEncoderPlugin" );

  d->flags = lame_init();

  if( d->flags == 0 ) {
    kdDebug() << "(K3bLameEncoder) lame_init failed." << endl;
    return false;
  }

  //
  // set the format of the input data
  //
  lame_set_num_samples( d->flags, length.lba()*588 );
  lame_set_in_samplerate( d->flags, 44100 );
  lame_set_num_channels( d->flags, 2 );

  //
  // Choose the quality level
  //
  if( c->readBoolEntry( "Manual Quality Settings", false ) ) {
    //
    // Mode
    //
    QString mode = c->readEntry( "Mode", "stereo" );
    if( mode == "stereo" )
      lame_set_mode( d->flags, STEREO );
    else if( mode == "joint" )
      lame_set_mode( d->flags, JOINT_STEREO );
    else // mono
      lame_set_mode( d->flags, MONO );

    //
    // Variable Bitrate
    //
    if( c->readBoolEntry( "VBR", false ) ) {
      // we use the default algorithm here
      lame_set_VBR( d->flags, vbr_default );
      
      if( c->readBoolEntry( "Manual Bitrate Settings", false ) ) {
	if( c->readBoolEntry( "Use Maximum Bitrate", false ) ) {
	  lame_set_VBR_max_bitrate_kbps( d->flags, c->readNumEntry( "Maximum Bitrate", 224 ) );
	}
	if( c->readBoolEntry( "Use Minimum Bitrate", false ) ) {
	  lame_set_VBR_min_bitrate_kbps( d->flags, c->readNumEntry( "Minimum Bitrate", 32 ) );
	  
	  // TODO: lame_set_hard_min
	}
	if( c->readBoolEntry( "Use Average Bitrate", true ) ) {
	  lame_set_VBR( d->flags, vbr_abr );
	  lame_set_VBR_mean_bitrate_kbps( d->flags, c->readNumEntry( "Average Bitrate", 128 ) );
	}
      }
    }

    //
    // Constant Bitrate
    //
    else {
      lame_set_VBR( d->flags, vbr_off );
      lame_set_brate( d->flags, c->readNumEntry( "Constant Bitrate", 128 ) );
    }
  }

  else {
    //
    // In lame 0 is the highest quality. Since that is just confusing for the user
    // if we call the setting "Quality" we simply invert the value.
    //
    int q = c->readNumEntry( "Quality Level", 5 );
    if( q < 0 ) q = 0;
    if( q > 9 ) q = 9;
    
    lame_set_preset( d->flags, s_lame_presets[q] );

    if( q < 2 )
      lame_set_mode( d->flags, MONO );
  }


  //
  // file options
  //  
  lame_set_copyright( d->flags, c->readBoolEntry( "Copyright", false ) );
  lame_set_original( d->flags, c->readBoolEntry( "Original", true ) );
  lame_set_strict_ISO( d->flags, c->readBoolEntry( "ISO compliance", false ) );
  lame_set_error_protection( d->flags, c->readBoolEntry( "Error Protection", false ) );


  //
  // Used Algorithm
  //
  // default to 2 which is the same as the -h lame option
  // THIS HAS NO INFLUENCE ON THE SIZE OF THE FILE!
  //
  //
  // In lame 0 is the highest quality. Since that is just confusing for the user
  // if we call the setting "Quality" we simply invert the value.
  //
  int q = c->readNumEntry( "Encoder Quality", 7 );
  if( q < 0 ) q = 0;
  if( q > 9 ) q = 9;
  lame_set_quality( d->flags, 9-q );

  //
  // ID3 settings
  //
  // for now we default to both v1 and v2 tags
  id3tag_add_v2( d->flags );
  id3tag_pad_v2( d->flags );

  return( lame_init_params( d->flags ) != -1 );
}


long K3bLameEncoder::encodeInternal( const char* data, Q_ULONG len )
{
  // FIXME: we may have to swap data here
  int size = lame_encode_buffer_interleaved( d->flags,
					     (short int*)data,
					     len/4,
					     (unsigned char*)d->buffer,
					     8000 );
  if( size < 0 ) {
    kdDebug() << "(K3bLameEncoder) lame_encode_buffer_interleaved failed." << endl;
    return -1;
  }

  return ::fwrite( d->buffer, 1, size, d->fid );
}


void K3bLameEncoder::finishEncoderInternal()
{
  int size = lame_encode_flush( d->flags,
				(unsigned char*)d->buffer,
				8000 );
  if( size > 0 )
    ::fwrite( d->buffer, 1, size, d->fid );

  lame_mp3_tags_fid( d->flags, d->fid );

  lame_close( d->flags );
  d->flags = 0;
}


void K3bLameEncoder::setMetaDataInternal( K3bAudioEncoder::MetaDataField f, const QString& value )
{
  // let's not use UTF-8 here since I don't know how to tell lame...
  // FIXME: when we use the codec we only get garbage. Why?
  QTextCodec* codec = 0;//QTextCodec::codecForName( "ISO8859-1" );
//  if( !codec )
//    kdDebug() << "(K3bLameEncoder) could not find codec ISO8859-1." << endl;

  switch( f ) {
  case META_TRACK_TITLE:
    id3tag_set_title( d->flags, codec ? codec->fromUnicode(value).data() : value.latin1() );
    break;
  case META_TRACK_ARTIST:
    id3tag_set_artist( d->flags, codec ? codec->fromUnicode(value).data() : value.latin1() );
    break;
  case META_ALBUM_TITLE:
    id3tag_set_album( d->flags, codec ? codec->fromUnicode(value).data() : value.latin1() );
    break;
  case META_ALBUM_COMMENT:
    id3tag_set_comment( d->flags, codec ? codec->fromUnicode(value).data() : value.latin1() );
    break;
  case META_YEAR:
    id3tag_set_year( d->flags, codec ? codec->fromUnicode(value).data() : value.latin1() );
    break;
  case META_TRACK_NUMBER:
    id3tag_set_track( d->flags, codec ? codec->fromUnicode(value).data() : value.latin1() );
    break;
  case META_GENRE:
    if( id3tag_set_genre( d->flags, codec ? codec->fromUnicode(value).data() : value.latin1() ) )
      kdDebug() << "(K3bLameEncoder) unable to set genre." << endl;
    break;
  default:
    return;
  }

  if( lame_init_params( d->flags ) < 0 )
    kdDebug() << "(K3bLameEncoder) lame_init_params failed." << endl;
}





K3bLameEncoderSettingsWidget::K3bLameEncoderSettingsWidget( QWidget* parent, const char* name )
  : K3bPluginConfigWidget( parent, name )
{
  m_w = new base_K3bLameEncoderSettingsWidget( this );
  m_w->m_sliderQuality->setRange( 0, 9 );
  m_w->m_spinEncoderQuality->setRange( 0, 9, true );

  m_manualSettingsDlg = new KDialogBase( this, 0, true,
					 i18n("(Lame) Manual Quality Settings") );
  m_brW = new base_K3bManualBitrateSettingsWidget( m_manualSettingsDlg );
  m_manualSettingsDlg->setMainWidget( m_brW );

  for( int i = 0; s_lame_bitrates[i]; ++i )
    m_brW->m_comboMaximumBitrate->insertItem( i18n("%1 kbps" ).arg(s_lame_bitrates[i]) );

  for( int i = 0; s_lame_bitrates[i]; ++i )
    m_brW->m_comboMinimumBitrate->insertItem( i18n("%1 kbps" ).arg(s_lame_bitrates[i]) );

  for( int i = 0; s_lame_bitrates[i]; ++i )
    m_brW->m_comboConstantBitrate->insertItem( i18n("%1 kbps" ).arg(s_lame_bitrates[i]) );


  QHBoxLayout* lay = new QHBoxLayout( this );
  lay->setMargin( 0 );
  lay->addWidget( m_w );

  // TODO: add whatsthis help for the quality level.
  //  QString qualityLevelWhatsThis = i18n("<p>");

  connect( m_w->m_buttonManualSettings, SIGNAL(clicked()),
	   this, SLOT(slotShowManualSettings()) );
  connect( m_w->m_sliderQuality, SIGNAL(valueChanged(int)),
	   this, SLOT(slotQualityLevelChanged(int)) );

  updateManualSettingsLabel();
  slotQualityLevelChanged( 5 );
}


K3bLameEncoderSettingsWidget::~K3bLameEncoderSettingsWidget()
{
}


void K3bLameEncoderSettingsWidget::slotShowManualSettings()
{
  // save current settings for proper cancellation
  bool constant = m_brW->m_radioConstantBitrate->isChecked();
  int constBitrate = m_brW->m_comboConstantBitrate->currentItem();
  int max = m_brW->m_comboMaximumBitrate->currentItem();
  int min = m_brW->m_comboMinimumBitrate->currentItem();
  int av = m_brW->m_spinAverageBitrate->value();
  int mode = m_brW->m_comboMode->currentItem();

  if( m_manualSettingsDlg->exec() == QDialog::Rejected ) {
    m_brW->m_radioConstantBitrate->setChecked( constant );
    m_brW->m_comboConstantBitrate->setCurrentItem( constBitrate );
    m_brW->m_comboMaximumBitrate->setCurrentItem( max );
    m_brW->m_comboMinimumBitrate->setCurrentItem( min );
    m_brW->m_spinAverageBitrate->setValue( av );
    m_brW->m_comboMode->setCurrentItem( mode );
  }
  else
    updateManualSettingsLabel();
}


void K3bLameEncoderSettingsWidget::updateManualSettingsLabel()
{
  if( m_brW->m_radioConstantBitrate->isChecked() )
    m_w->m_labelManualSettings->setText( i18n("Constant Bitrate: %1 kbps (%2)")
					 .arg(s_lame_bitrates[m_brW->m_comboConstantBitrate->currentItem()])
					 .arg(i18n(s_lame_mode_strings[m_brW->m_comboMode->currentItem()])) );
  else
    m_w->m_labelManualSettings->setText( i18n("Variable Bitrate (%1)")
					 .arg(i18n(s_lame_mode_strings[m_brW->m_comboMode->currentItem()])) );
}


void K3bLameEncoderSettingsWidget::slotQualityLevelChanged( int val )
{
  m_w->m_labelQualityLevel->setText( i18n(s_lame_preset_strings[val]) );
}


void K3bLameEncoderSettingsWidget::loadConfig()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "K3bLameEncoderPlugin" );

  QString mode = c->readEntry( "Mode", "stereo" );
  if( mode == "stereo" )
    m_brW->m_comboMode->setCurrentItem( 0 );
  else if( mode == "joint" )
    m_brW->m_comboMode->setCurrentItem( 1 );
  else // mono
    m_brW->m_comboMode->setCurrentItem( 2 );

  bool manual = c->readBoolEntry( "Manual Bitrate Settings", false );
  if( manual )
    m_w->m_radioManual->setChecked(true);
  else
    m_w->m_radioQualityLevel->setChecked(true);

  if( c->readBoolEntry( "VBR", false ) )
    m_brW->m_radioVariableBitrate->setChecked( true );
  else
    m_brW->m_radioConstantBitrate->setChecked( true );

  m_brW->m_comboConstantBitrate->setCurrentItem( i18n("%1 kbps").arg(c->readNumEntry( "Constant Bitrate", 128 )) );
  m_brW->m_comboMaximumBitrate->setCurrentItem( i18n("%1 kbps").arg(c->readNumEntry( "Maximum Bitrate", 224 )) );
  m_brW->m_comboMinimumBitrate->setCurrentItem( i18n("%1 kbps").arg(c->readNumEntry( "Minimum Bitrate", 32 )) );
  m_brW->m_spinAverageBitrate->setValue( c->readNumEntry( "Average Bitrate", 128) );

  m_brW->m_checkBitrateMaximum->setChecked( c->readBoolEntry( "Use Maximum Bitrate", false ) );
  m_brW->m_checkBitrateMinimum->setChecked( c->readBoolEntry( "Use Minimum Bitrate", false ) );
  m_brW->m_checkBitrateAverage->setChecked( c->readBoolEntry( "Use Average Bitrate", true ) );

  m_w->m_sliderQuality->setValue( c->readNumEntry( "Quality Level", 5 ) );

  m_w->m_checkCopyright->setChecked( c->readBoolEntry( "Copyright", false ) );
  m_w->m_checkOriginal->setChecked( c->readBoolEntry( "Original", true ) );
  m_w->m_checkISO->setChecked( c->readBoolEntry( "ISO compliance", false ) );
  m_w->m_checkError->setChecked( c->readBoolEntry( "Error Protection", false ) );

  // default to 2 which is the same as the -h lame option
  m_w->m_spinEncoderQuality->setValue( c->readNumEntry( "Encoder Quality", 7 ) );

  updateManualSettingsLabel();
}


void K3bLameEncoderSettingsWidget::saveConfig()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "K3bLameEncoderPlugin" );

  QString mode;
  switch( m_brW->m_comboMode->currentItem() ) {
  case 0:
    mode = "stereo";
    break;
  case 1:
    mode = "joint";
    break;
  case 2:
    mode = "mono";
    break;
  }
  c->writeEntry( "Mode", mode );

  c->writeEntry( "Manual Bitrate Settings", m_w->m_radioManual->isChecked() );

  c->writeEntry( "VBR", !m_brW->m_radioConstantBitrate->isChecked() );
  c->writeEntry( "Constant Bitrate", m_brW->m_comboConstantBitrate->currentText().left(3).toInt() );
  c->writeEntry( "Maximum Bitrate", m_brW->m_comboMaximumBitrate->currentText().left(3).toInt() );
  c->writeEntry( "Minimum Bitrate", m_brW->m_comboMinimumBitrate->currentText().left(3).toInt() );
  c->writeEntry( "Average Bitrate", m_brW->m_spinAverageBitrate->value() );

  c->writeEntry( "Use Maximum Bitrate", m_brW->m_checkBitrateMaximum->isChecked() );
  c->writeEntry( "Use Minimum Bitrate", m_brW->m_checkBitrateMinimum->isChecked() );
  c->writeEntry( "Use Average Bitrate", m_brW->m_checkBitrateAverage->isChecked() );

  c->writeEntry( "Quality Level", m_w->m_sliderQuality->value() );

  c->writeEntry( "Copyright", m_w->m_checkCopyright->isChecked() );
  c->writeEntry( "Original", m_w->m_checkOriginal->isChecked() );
  c->writeEntry( "ISO compliance", m_w->m_checkISO->isChecked() );
  c->writeEntry( "Error Protection", m_w->m_checkError->isChecked() );

  // default to 2 which is the same as the -h lame option
  c->writeEntry( "Encoder Quality", m_w->m_spinEncoderQuality->value() );
}



QStringList K3bLameEncoder::extensions() const
{
  return QStringList( "mp3" );
}


QString K3bLameEncoder::fileTypeComment( const QString& ) const
{
  return "MPEG1 Layer III (mp3)";
}


long long K3bLameEncoder::fileSize( const QString&, const K3b::Msf& msf ) const
{
  // FIXME!
  KConfig* c = k3bcore->config();
  c->setGroup( "K3bLameEncoderPlugin" );
  int bitrate = c->readNumEntry( "Constant Bitrate", 128 );

  return (msf.totalFrames()/75 * bitrate * 1000)/8;
}


K3bPluginConfigWidget* K3bLameEncoder::createConfigWidget( QWidget* parent, 
							   const char* name ) const
{
  return new K3bLameEncoderSettingsWidget( parent, name );
}


#include "k3blameencoder.moc"
