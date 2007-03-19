/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvideodvdrippingdialog.h"
#include "k3bvideodvdrippingwidget.h"
#include <k3bvideodvdtitletranscodingjob.h>
#include <k3bjobprogressdialog.h>
#include <k3bapplication.h>
#include <k3bmedium.h>
#include <k3bmediacache.h>
#include <k3bglobals.h>
#include <k3bfilesysteminfo.h>

#include <klocale.h>
#include <klistview.h>
#include <klocale.h>
#include <kglobal.h>
#include <kurlrequester.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kio/global.h>
#include <kconfig.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qstyle.h>
#include <qfontmetrics.h>


static QString videoCodecId( K3bVideoDVDTitleTranscodingJob::VideoCodec codec )
{
  switch( codec ) {
  case K3bVideoDVDTitleTranscodingJob::VIDEO_CODEC_FFMPEG_MPEG4:
    return "ffmpeg_mpeg4";
  case K3bVideoDVDTitleTranscodingJob::VIDEO_CODEC_XVID:
    return "xvid";
  default:
    return "none";
  }
}


static QString audioCodecId( K3bVideoDVDTitleTranscodingJob::AudioCodec codec )
{
  switch( codec ) {
  case K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3:
    return "mp3";
  case K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_STEREO:
    return "ac3_stereo";
  case K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_PASSTHROUGH:
    return "ac3_passthrough";
  default:
    return "none";
  }
}


static K3bVideoDVDTitleTranscodingJob::VideoCodec videoCodecFromId( const QString& codec )
{
  if( codec == "xvid" )
    return K3bVideoDVDTitleTranscodingJob::VIDEO_CODEC_XVID;
  else //  if( codec == "ffmpeg_mpeg4" )
    return K3bVideoDVDTitleTranscodingJob::VIDEO_CODEC_FFMPEG_MPEG4;
}


static K3bVideoDVDTitleTranscodingJob::AudioCodec audioCodecFromId( const QString& codec )
{
  if( codec == "ac3_stereo" )
    return K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_STEREO;
  else if( codec == "ac3_passthrough" )
    return K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_PASSTHROUGH;
  else // if( codec == "mp3" )
    return K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3;
}


// resize according to aspect ratio
static QSize resizeTitle( const K3bVideoDVD::VideoStream& title, const QSize& size )
{
  int w = size.width();
  int h = size.height();
  int rw = title.realPictureWidth();
  int rh = title.realPictureHeight();

  if( w == 0 && h == 0 ) {
    w = rw;
    h = rh;
  }
  else if( w == 0 ) {
    w = h * rw / rh;
  }
  else if( h == 0 ) {
    h = w * rh / rw;
  }

  return QSize(w,h);
}



class K3bVideoDVDRippingDialog::AudioStreamViewItem : public QCheckListItem
{
public:
  AudioStreamViewItem( K3bVideoDVDRippingDialog* dlg,
		       QCheckListItem* parent, QListViewItem* after, const QString& text,
		       int audioStream )
    : QCheckListItem( parent, after, text, RadioButton ),
      m_audioStream( audioStream ),
      m_dlg( dlg ) {
  }

private:
  void stateChange( bool ) {
    if( state() == On ) {
      m_dlg->m_titleRipInfos[static_cast<QCheckListItem*>(parent())].audioStream = m_audioStream;
      m_dlg->slotUpdateFilenames();
    }
  }

  int m_audioStream;
  K3bVideoDVDRippingDialog* m_dlg;
};


class K3bVideoDVDRippingDialog::Private
{
public:
  K3bFileSystemInfo fsInfo;
};


K3bVideoDVDRippingDialog::K3bVideoDVDRippingDialog( const K3bVideoDVD::VideoDVD& dvd,
						    const QValueList<int>& titles,
						    QWidget* parent, const char* name )
  : K3bInteractionDialog( parent, name,
			  i18n("Video DVD Ripping"),
			  QString::null,
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  "VideoDVD Ripping" ), // config group
    m_dvd( dvd )
{
  d = new Private;

  QWidget* frame = mainWidget();
  QHBoxLayout* frameLayout = new QHBoxLayout( frame );
  frameLayout->setMargin( 0 );
  frameLayout->setAutoAdd( true );
  m_w = new K3bVideoDVDRippingWidget( frame );

  connect( m_w, SIGNAL(changed()),
	   this, SLOT(slotUpdateFilesizes()) );
  connect( m_w, SIGNAL(changed()),
	   this, SLOT(slotUpdateFilenames()) );
  connect( m_w, SIGNAL(changed()),
	   this, SLOT(slotUpdateVideoSizes()) );

  setTitle( i18n("Video DVD Ripping"),
	    i18n("1 title from %1", "%n titles from %1", titles.count())
	    .arg( k3bappcore->mediaCache()->medium(m_dvd.device()).beautifiedVolumeId() ) );

  // populate list map
  populateTitleView( titles );
}


K3bVideoDVDRippingDialog::~K3bVideoDVDRippingDialog()
{
  delete d;
}


void K3bVideoDVDRippingDialog::populateTitleView( const QValueList<int>& titles )
{
  m_w->m_titleView->clear();
  m_titleRipInfos.clear();

  QCheckListItem* titleItem = 0;
  for( QValueList<int>::const_iterator it = titles.begin(); it != titles.end(); ++it ) {
    titleItem = new QCheckListItem( m_w->m_titleView,
				    titleItem,
				    i18n("Title %1 (%2)")
				    .arg(*it)
				    .arg(m_dvd[*it-1].playbackTime().toString()),
				    QCheckListItem::RadioButtonController );
    titleItem->setText( 1, QString("%1x%2")
		       .arg(m_dvd[*it-1].videoStream().realPictureWidth())
		       .arg(m_dvd[*it-1].videoStream().realPictureHeight()) );
    titleItem->setText( 3, QString("%1 Title %2.avi").arg(m_dvd.volumeIdentifier()).arg(*it) );

    // now for the rip info
    K3bVideoDVDRippingJob::TitleRipInfo ri( *it );

    //
    // Determine default language selection:
    // first try the configured locale, if that fails, fall back to the first audio stream
    //
    ri.audioStream = 0;
    for( unsigned int i = 0; i < m_dvd[*it-1].numAudioStreams(); ++i ) {
      if( m_dvd[*it-1].audioStream(i).langCode() == KGlobal::locale()->language() &&
	  m_dvd[*it-1].audioStream(i).format() != K3bVideoDVD::AUDIO_FORMAT_DTS ) {
	ri.audioStream = i;
	break;
      }
    }

    QListViewItem* asI = 0;
    for( unsigned int i = 0; i < m_dvd[*it-1].numAudioStreams(); ++i ) {
      QString text = i18n("%1 %2Ch (%3%4)")
	.arg( K3bVideoDVD::audioFormatString( m_dvd[*it-1].audioStream(i).format() ) )
	.arg( m_dvd[*it-1].audioStream(i).channels() )
	.arg( m_dvd[*it-1].audioStream(i).langCode().isEmpty()
	      ? i18n("unknown language")
	      : KGlobal::locale()->twoAlphaToLanguageName( m_dvd[*it-1].audioStream(i).langCode() ) )
	.arg( m_dvd[*it-1].audioStream(i).codeExtension() != K3bVideoDVD::AUDIO_CODE_EXT_UNSPECIFIED
	      ? QString(" ") + K3bVideoDVD::audioCodeExtensionString( m_dvd[*it-1].audioStream(i).codeExtension() )
	      : QString::null );

      if( m_dvd[*it-1].audioStream(i).format() == K3bVideoDVD::AUDIO_FORMAT_DTS ) {
	// width of the radio button from QCheckListItem::paintCell
	int buttonSize = style().pixelMetric( QStyle::PM_CheckListButtonSize, m_w->m_titleView ) + 4;
	int spaceWidth = fontMetrics().width( ' ' );
	int numSpaces = buttonSize/spaceWidth;
	asI = new QListViewItem( titleItem, asI, QString().fill( ' ', numSpaces ) + text + " (" + i18n("not supported") + ")" );
      }
      else {
	asI = new AudioStreamViewItem( this, titleItem, asI, text, i );

	if( ri.audioStream == (int)i )
	  ((AudioStreamViewItem*)asI)->setState( QCheckListItem::On );
      }
    }

    titleItem->setOpen( true );

    m_titleRipInfos[titleItem] = ri;
  }
}


void K3bVideoDVDRippingDialog::slotUpdateFilenames()
{
  QString baseDir = K3b::prepareDir( m_w->m_editBaseDir->url() );
  d->fsInfo.setPath( baseDir );

  for( QMap<QCheckListItem*, K3bVideoDVDRippingJob::TitleRipInfo>::iterator it = m_titleRipInfos.begin();
       it != m_titleRipInfos.end(); ++it ) {
    QString f = d->fsInfo.fixupPath( createFilename( it.data(), m_w->m_comboFilenamePattern->currentText() ) );
    if( m_w->m_checkBlankReplace->isChecked() )
      f.replace( QRegExp( "\\s" ), m_w->m_editBlankReplace->text() );
    it.data().filename = baseDir + f;
    it.key()->setText( 3, f );
  }
}


void K3bVideoDVDRippingDialog::slotUpdateFilesizes()
{
  double bitrate = (double)m_w->m_spinVideoBitrate->value();
  KIO::filesize_t overallSize = 0ULL;

  // update file sizes
  for( QMap<QCheckListItem*, K3bVideoDVDRippingJob::TitleRipInfo>::iterator it = m_titleRipInfos.begin();
       it != m_titleRipInfos.end(); ++it ) {

    double sec = m_dvd[it.data().title-1].playbackTime().totalSeconds();

    // estimate the filesize
    KIO::filesize_t size = (KIO::filesize_t)( sec * bitrate * 1000.0 / 8.0 );

    // add audio stream size
    // FIXME: consider AC3 passthrough
    size += (KIO::filesize_t)( sec * m_w->selectedAudioBitrate() / 8.0 * 1024.0 );

    it.key()->setText( 2, KIO::convertSize( size ) );

    overallSize += size;
  }

  m_w->setNeededSize( overallSize );
}


void K3bVideoDVDRippingDialog::slotUpdateVideoSizes()
{
  QSize size = m_w->selectedPictureSize();
  for( QMap<QCheckListItem*, K3bVideoDVDRippingJob::TitleRipInfo>::iterator it = m_titleRipInfos.begin();
       it != m_titleRipInfos.end(); ++it ) {
    QSize s( resizeTitle( m_dvd[it.data().title-1].videoStream(), size ) );
    it.key()->setText( 1, QString("%1x%2").arg(s.width()).arg(s.height()) );
  }
}


void K3bVideoDVDRippingDialog::setBaseDir( const QString& path )
{
  m_w->m_editBaseDir->setURL( path );
}


QString K3bVideoDVDRippingDialog::createFilename( const K3bVideoDVDRippingJob::TitleRipInfo& info, const QString& pattern ) const
{
  QString f;

  const K3bVideoDVD::Title& title = m_dvd[info.title-1];

  for( unsigned int i = 0; i < pattern.length(); ++i ) {
    //
    // every pattern starts with a % sign
    //
    if( pattern[i] == '%' ) {
      ++i; // skip the %
      QChar c = pattern[i];

      //
      // first check if we have a long keyword instead of a one-char
      //
      if( pattern[i] == '{' ) {
	int j = pattern.find( '}', i );
	if( j < 0 ) // no closing bracket -> no valid pattern
	  c = '*';
	else {
	  QString keyword = pattern.mid( i+1, j-i-1 );
	  if( keyword == "titlenumber"  ||
	      keyword == "title_number" ||
	      keyword == "title" ) {
	    c = PATTERN_TITLE_NUMBER;
	  }
	  else if( keyword == "volumeid"  ||
		   keyword == "volume_id" ||
		   keyword == "volid"     ||
		   keyword == "vol_id" ) {
	    c = PATTERN_VOLUME_ID;
	  }
	  else if( keyword == "beautifiedvolumeid"   ||
		   keyword == "beautified_volumeid"  ||
		   keyword == "beautified_volume_id" ||
		   keyword == "beautifiedvolid"      ||
		   keyword == "beautified_volid"     ||
		   keyword == "beautified_vol_id"    ||
		   keyword == "nicevolid"            ||
		   keyword == "nice_volid"           ||
		   keyword == "nice_vol_id" ) {
	    c = PATTERN_BEAUTIFIED_VOLUME_ID;
	  }
	  else if( keyword == "languagecode"  ||
		   keyword == "language_code" ||
		   keyword == "langcode"      ||
		   keyword == "lang_code" ) {
	    c = PATTERN_LANGUAGE_CODE;

	  }
	  else if( keyword == "lang" ||
		   keyword == "language" ||
		   keyword == "langname" ||
		   keyword == "languagename" ||
		   keyword == "lang_name" ||
		   keyword == "language_name" ) {
	    c = PATTERN_LANGUAGE_NAME;
	  }
	  else if( keyword == "audioformat"  ||
		   keyword == "audio_format" ||
		   keyword == "audio" ) {
	    c = PATTERN_AUDIO_FORMAT;
	  }
	  else if( keyword == "channels" ||
		   keyword == "audiochannels" ||
		   keyword == "audio_channels" ||
		   keyword == "ch" ) {
	    c = PATTERN_AUDIO_CHANNELS;
	  }
	  else if( keyword == "videosize"  ||
		   keyword == "video_size" ||
		   keyword == "vsize" ) {
	    c = PATTERN_VIDEO_SIZE;
	  }
	  else if( keyword == "originalvideosize"  ||
		   keyword == "original_video_size" ||
		   keyword == "origvideosize"  ||
		   keyword == "orig_video_size" ||
		   keyword == "origvsize" ) {
	    c = PATTERN_ORIG_VIDEO_SIZE;
	  }
	  else if( keyword == "aspect_ratio" ||
		   keyword == "aspectratio" ||
		   keyword == "ratio" ) {
	    c = PATTERN_ASPECT_RATIO;
	  }
	  else if( keyword == "current_date" ||
		   keyword == "currentdate" ||
		   keyword == "date" ) {
	    c = PATTERN_CURRENT_DATE;
	  }
	  else {
	    // unusable pattern
	    c = '*';
	  }

	  //
	  // skip the keyword and the closing bracket
	  //
	  if( c != '*' ) {
	    i += keyword.length() + 1;
	  }
	}
      }

      switch( c ) {
      case PATTERN_TITLE_NUMBER:
	f.append( QString::number(info.title).rightJustify( 2, '0' ) );
	break;
      case PATTERN_VOLUME_ID:
	f.append( m_dvd.volumeIdentifier() );
	break;
      case PATTERN_BEAUTIFIED_VOLUME_ID:
	f.append( k3bappcore->mediaCache()->medium( m_dvd.device() ).beautifiedVolumeId() );
	break;
      case PATTERN_LANGUAGE_CODE:
	if( title.numAudioStreams() > 0 )
	  f.append( title.audioStream( info.audioStream ).langCode() );
	break;
      case PATTERN_LANGUAGE_NAME:
	if( title.numAudioStreams() > 0 )
	  f.append( KGlobal::locale()->twoAlphaToLanguageName( title.audioStream( info.audioStream ).langCode() ) );
	break;
      case PATTERN_AUDIO_FORMAT:
	// FIXME: what about MPEG audio streams?
	if( title.numAudioStreams() > 0 ) {
	  if( m_w->selectedAudioCodec() == K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3 )
	    f.append( K3bVideoDVDTitleTranscodingJob::audioCodecString( m_w->selectedAudioCodec() ) );
	  else
	    f.append( K3bVideoDVD::audioFormatString( title.audioStream( info.audioStream ).format() ) );
	}
	break;
      case PATTERN_AUDIO_CHANNELS:
	if( title.numAudioStreams() > 0 )
	  f.append( i18n("%nCh", "%nCh",
			 m_w->selectedAudioCodec() == K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_PASSTHROUGH
			 ? title.audioStream( info.audioStream ).channels()
			 : 2 ) );
	break;
      case PATTERN_ORIG_VIDEO_SIZE:
	f.append( QString("%1x%2")
		  .arg(title.videoStream().pictureWidth())
		  .arg(title.videoStream().pictureHeight()) );
	break;
      case PATTERN_VIDEO_SIZE: {
	QSize s( resizeTitle( m_dvd[info.title-1].videoStream(), m_w->selectedPictureSize() ) );
	f.append( QString("%1x%2").arg(s.width()).arg(s.height()) );
	break;
      }
      case PATTERN_ASPECT_RATIO:
	if( title.videoStream().displayAspectRatio() == K3bVideoDVD::VIDEO_ASPECT_RATIO_4_3 )
	  f.append( "4:3" );
	else
	  f.append( "16:9" );
	break;
      case PATTERN_CURRENT_DATE:
	f.append( KGlobal::locale()->formatDate( QDate::currentDate() ) );
	break;
      default:
	f.append( pattern[i-1] );
	f.append( pattern[i] );
      }
    }

    //
    // normal character -> just append to filename
    //
    else {
      f.append( pattern[i] );
    }
  }

  //
  // and the extension (for now only avi)
  //
  f.append( ".avi" );

  return f;
}


void K3bVideoDVDRippingDialog::loadK3bDefaults()
{
  m_w->m_spinVideoBitrate->setValue( 1800 );
  m_w->m_checkTwoPassEncoding->setChecked( true );
  m_w->m_checkAudioResampling->setChecked( false );
  m_w->m_checkAutoClipping->setChecked( false );
  m_w->m_checkLowPriority->setChecked( true );
  m_w->m_checkAudioVBR->setChecked( true );
  m_w->setSelectedAudioBitrate( 128 );
  m_w->setSelectedVideoCodec( K3bVideoDVDTitleTranscodingJob::VIDEO_CODEC_FFMPEG_MPEG4 );
  m_w->setSelectedAudioCodec( K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3 );
  m_w->m_checkBlankReplace->setChecked( false );
  m_w->m_editBlankReplace->setText( "_" );
  m_w->m_comboFilenamePattern->setEditText( m_w->m_comboFilenamePattern->text(0) );
  m_w->m_editBaseDir->setURL( K3b::defaultTempPath() );
}


void K3bVideoDVDRippingDialog::loadUserDefaults( KConfigBase* c )
{
  m_w->m_spinVideoBitrate->setValue( c->readNumEntry( "video bitrate", 1200 ) );
  m_w->m_checkTwoPassEncoding->setChecked( c->readBoolEntry( "two pass encoding", true ) );
  m_w->m_checkAudioResampling->setChecked( c->readBoolEntry( "audio resampling", false ) );
  m_w->m_checkAutoClipping->setChecked( c->readBoolEntry( "auto clipping", false ) );
  m_w->m_checkLowPriority->setChecked( c->readBoolEntry( "low priority", true ) );
  m_w->m_checkAudioVBR->setChecked( c->readBoolEntry( "vbr audio", true ) );
  m_w->setSelectedAudioBitrate( c->readNumEntry( "audio bitrate", 128 ) );
  m_w->setSelectedVideoCodec( videoCodecFromId( c->readEntry( "video codec", videoCodecId( K3bVideoDVDTitleTranscodingJob::VIDEO_CODEC_FFMPEG_MPEG4 ) ) ) );
  m_w->setSelectedAudioCodec( audioCodecFromId( c->readEntry( "audio codec", audioCodecId( K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3 ) ) ) );
  m_w->m_checkBlankReplace->setChecked( c->readBoolEntry( "replace blanks", false ) );
  m_w->m_editBlankReplace->setText( c->readEntry( "blank replace string", "_" ) );
  m_w->m_comboFilenamePattern->setEditText( c->readEntry( "filename pattern", m_w->m_comboFilenamePattern->text(0) ) );
  m_w->m_editBaseDir->setURL( c->readPathEntry( "base dir", K3b::defaultTempPath() ) );
}


void K3bVideoDVDRippingDialog::saveUserDefaults( KConfigBase* c )
{
  c->writeEntry( "video bitrate", m_w->m_spinVideoBitrate->value() );
  c->writeEntry( "two pass encoding", m_w->m_checkTwoPassEncoding->isChecked() );
  c->writeEntry( "audio resampling", m_w->m_checkAudioResampling->isChecked() );
  c->writeEntry( "auto clipping", m_w->m_checkAutoClipping->isChecked() );
  c->writeEntry( "low priority", m_w->m_checkLowPriority->isChecked() );
  c->writeEntry( "vbr audio", m_w->m_checkAudioVBR->isChecked() );
  c->writeEntry( "audio bitrate", m_w->selectedAudioBitrate() );
  c->writeEntry( "video codec", videoCodecId( m_w->selectedVideoCodec() ) );
  c->writeEntry( "audio codec", audioCodecId( m_w->selectedAudioCodec() ) );
  c->writeEntry( "replace blanks", m_w->m_checkBlankReplace->isChecked() );
  c->writeEntry( "blank replace string", m_w->m_editBlankReplace->text() );
  c->writeEntry( "filename pattern", m_w->m_comboFilenamePattern->currentText() );
  c->writePathEntry( "base dir", m_w->m_editBaseDir->url() );
}


void K3bVideoDVDRippingDialog::slotStartClicked()
{
  //
  // check if the selected audio codec is usable for all selected audio streams
  // We can only use the AC3 pass-through mode for AC3 streams
  //
  if( m_w->selectedAudioCodec() == K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_PASSTHROUGH ) {
    for( QMap<QCheckListItem*, K3bVideoDVDRippingJob::TitleRipInfo>::iterator it = m_titleRipInfos.begin();
	 it != m_titleRipInfos.end(); ++it ) {
      if( m_dvd[it.data().title-1].numAudioStreams() > 0 &&
          m_dvd[it.data().title-1].audioStream(it.data().audioStream).format() != K3bVideoDVD::AUDIO_FORMAT_AC3 ) {
	KMessageBox::sorry( this, i18n("<p>When using the <em>AC3 pass-through</em> audio codec all selected audio "
				       "streams need to be in AC3 format. Please select another audio codec or "
				       "choose AC3 audio streams for all ripped titles."),
			    i18n("AC3 Pass-through") );
	return;
      }
    }
  }

  // check if we need to overwrite some files...
  QStringList filesToOverwrite;
  for( QMap<QCheckListItem*, K3bVideoDVDRippingJob::TitleRipInfo>::iterator it = m_titleRipInfos.begin();
       it != m_titleRipInfos.end(); ++it ) {
    if( QFile::exists( it.data().filename ) )
      filesToOverwrite.append( it.data().filename );
  }

  if( !filesToOverwrite.isEmpty() )
    if( KMessageBox::questionYesNoList( this,
					i18n("Do you want to overwrite these files?"),
					filesToOverwrite,
					i18n("Files Exist"), i18n("Overwrite"), KStdGuiItem::cancel() ) == KMessageBox::No )
      return;


  QSize videoSize = m_w->selectedPictureSize();
  int i = 0;
  QValueVector<K3bVideoDVDRippingJob::TitleRipInfo> titles( m_titleRipInfos.count() );
  for( QMapConstIterator<QCheckListItem*, K3bVideoDVDRippingJob::TitleRipInfo> it = m_titleRipInfos.begin();
       it != m_titleRipInfos.end(); ++it ) {
    titles[i] = it.data();
    titles[i].videoBitrate = 0; // use the global bitrate set below
    titles[i].width = videoSize.width();
    titles[i].height = videoSize.height();
    ++i;
  }

  // sort the titles which come from a map and are thus not sorted properly
  // simple bubble sort for these small arrays is sufficient
  for( unsigned int i = 0; i < titles.count(); ++i ) {
    for( unsigned int j = i+1; j < titles.count(); ++j ) {
      if( titles[i].title > titles[j].title ) {
	K3bVideoDVDRippingJob::TitleRipInfo tmp = titles[i];
	titles[i] = titles[j];
	titles[j] = tmp;
      }
    }
  }

  // start the job
  K3bJobProgressDialog dlg( parentWidget() );
  K3bVideoDVDRippingJob* job = new K3bVideoDVDRippingJob( &dlg, &dlg );
  job->setVideoDVD( m_dvd );
  job->setTitles( titles );

  job->setVideoBitrate( m_w->m_spinVideoBitrate->value() );
  job->setTwoPassEncoding( m_w->m_checkTwoPassEncoding->isChecked() );
  job->setResampleAudioTo44100( m_w->m_checkAudioResampling->isChecked() );
  job->setAutoClipping( m_w->m_checkAutoClipping->isChecked() );
  job->setVideoCodec( m_w->selectedVideoCodec() );
  job->setAudioCodec( m_w->selectedAudioCodec() );
  job->setLowPriority( m_w->m_checkLowPriority->isChecked() );
  job->setAudioBitrate( m_w->selectedAudioBitrate() );
  job->setAudioVBR( m_w->m_checkAudioVBR->isChecked() );

  hide();
  dlg.startJob( job );
  close();
}

#include "k3bvideodvdrippingdialog.moc"
