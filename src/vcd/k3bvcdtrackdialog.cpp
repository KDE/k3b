/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include <qlineedit.h>
#include <qmultilineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qgroupbox.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qhbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kio/global.h>

#include "k3bvcdtrackdialog.h"
#include "k3bvcdtrack.h"
#include "../kcutlabel.h"
#include "../tools/k3bglobals.h"
#include "../device/k3bmsf.h"


K3bVcdTrackDialog::K3bVcdTrackDialog( QPtrList<K3bVcdTrack>& tracks, QWidget *parent, const char *name )
  : KDialogBase( KDialogBase::Plain, i18n("Video Track Properties"), KDialogBase::Ok,
		 KDialogBase::Ok, parent, name )
{
  prepareGui();
  
  setupNavigationTab();
  setupVideoTab();
  setupAudioTab();

  m_tracks = tracks;

  if( !m_tracks.isEmpty() ) {

    K3bVcdTrack* track = m_tracks.first();

    m_displayFileName->setText( track->fileName() );
    m_displayLength->setText( track->mpegDuration() );
    m_displaySize->setText( KIO::convertSize(track->size()) );

    m_labelMimeType->setPixmap( KMimeType::pixmapForURL( KURL(m_tracks.first()->absPath()), 0, KIcon::Desktop, 48 ) );

    fillGui();
  }
}

K3bVcdTrackDialog::~K3bVcdTrackDialog()
{
}


void K3bVcdTrackDialog::slotOk()
{
  slotApply();
  done(0);
}

void K3bVcdTrackDialog::fillGui()
{
  QString tmp;
  K3bVcdTrack* track = m_tracks.first();
        
  if (track->mpegVideoVersion() == 1) {
    if (track->hasAudio())
      m_mpegver_video->setText(i18n("Mpeg 1 System File [Video/Audio]"));
    else
      m_mpegver_video->setText(i18n("Mpeg 1 System File [Video]"));
  }
  else {
    if (track->hasAudio())
      m_mpegver_video->setText(i18n("Mpeg Program Stream File [Video/Audio]"));
    else
      m_mpegver_video->setText(i18n("Mpeg Program Stream File [Video]"));
  }

  m_rate_video->setText(i18n("%1 Mbps").arg(track->mpegMbps()));

  m_duration_video->setText(track->mpegDuration());

  switch (track->MpegAspectRatio()) {
    case 0: m_rate_video->setText(i18n("Invalid aspect ratio (forbidden)")); break;
    case 1: m_rate_video->setText(i18n("Aspect ratio 1/1 (VGA)")); break;
    case 2: m_rate_video->setText(i18n("Aspect ratio 4/3 (TV)")); break;
    case 3: m_rate_video->setText(i18n("Aspect ratio 16/9 (large TV)")); break;
    case 4: m_rate_video->setText(i18n("Aspect ratio 2.21/1 (Cinema)")); break;
    default: m_rate_video->setText(i18n("Invalid Aspect ratio (reserved)"));
  }

  m_chromaformat_video->setText(i18n("n/a"));

  if (track->MpegSExt()){
    if (track->MpegProgressive())
      tmp = i18n("Not interlaced");
    else
      tmp = i18n("Interlaced");

    switch (track->MpegChromaFormat()){
      case 1 : tmp.append(", 4:2:0");break;
      case 2 : tmp.append(", 4:2:2");break;
      case 3 : tmp.append(", 4:4:4");break;
    }
    m_chromaformat_video->setText(tmp);
  }

  m_format_video->setText(i18n("n/a"));
  if (track->MpegDExt()){
    switch(track->MpegFormat()){
      case 0 : m_format_video->setText(i18n("Component"));break;
      case 1 : m_format_video->setText("PAL");break;
      case 2 : m_format_video->setText("NTSC");break;
      case 3 : m_format_video->setText("SECAM");break;
      case 4 : m_format_video->setText("MAC");break;
      case 5 : m_format_video->setText(i18n("Unspecified"));break;
    }
  }

  m_displaysize_video->setText(track->mpegDisplaySize());
  m_size_video->setText(i18n("%1 %2 fps %3 Mbps").arg(track->mpegSize()).arg(track->mpegFps()).arg(track->mpegMbps()));
  

  if (track->hasAudio()){
    if (track->MpegAudioType() !=3)
      m_mpegver_audio->setText(i18n("Mpeg %1 layer %2").arg(track->MpegAudioType()).arg(track->MpegAudioLayer()));
    else
      m_mpegver_audio->setText(i18n("Mpeg 2.5 (rare) layer %1").arg(track->MpegAudioLayer()));

    if (!track->MpegAudioKbps().isNull())
      m_rate_audio->setText(i18n("%1 kbps %2 Hz").arg(track->MpegAudioKbps()).arg(track->MpegAudioHz()));
    else
      m_rate_audio->setText(i18n("free bitrate %1 Hz").arg(track->MpegAudioHz()));

    switch (track->MpegAudioMode()){
      case 0: tmp = i18n("Stereo"); break;
      case 1: tmp = i18n("Joint Stereo");
              if (track->MpegAudioLayer() == 1 || track->MpegAudioLayer() == 2){
                switch (track->MpegAudioModeExt()){
                  case 0: tmp.append(" " + i18n("(Intensity stereo on bands 4-31/32)")); break;
                  case 1: tmp.append(" " + i18n("(Intensity stereo on bands 8-31/32)")); break;
                  case 2: tmp.append(" " + i18n("(Intensity stereo on bands 12-31/32)")); break;
                  case 3: tmp.append(" " + i18n("(Intensity stereo on bands 16-31/32)")); break;
                }
              }
              else {
                //mp3
                switch (track->MpegAudioModeExt()){
                  case 0: tmp.append(" " + i18n("(Intensity stereo off, M/S stereo off)")); break;
                  case 1: tmp.append(" " + i18n("(Intensity stereo on, M/S stereo off)")); break;
                  case 2: tmp.append(" " + i18n("(Intensity stereo off, M/S stereo on)")); break;
                  case 3: tmp.append(" " + i18n("(Intensity stereo on, M/S stereo on)")); break;
                }
              }
              break;
      case 2: tmp = i18n("Dual Channel"); break;
      case 3: tmp = i18n("Mono"); break;
    }
    m_mode_audio->setText(tmp);

    switch (track->MpegAudioEmphasis()){
      case 0: m_emphasis_audio->setText(i18n("No emphasis")); break;
      case 1: m_emphasis_audio->setText(i18n("Emphasis 50/15 microsecs")); break;
      case 2: m_emphasis_audio->setText(i18n("Emphasis Unknown")); break;
      case 3: m_emphasis_audio->setText(i18n("Emphasis CCITT J 17")); break;
    }

    tmp = "";
    if (track->MpegAudioCopyright()) tmp.append("(c) ");
    if (track->MpegAudioOriginal())
      tmp.append(i18n("original"));
    else
      tmp.append(i18n("duplicate"));

    m_copyright_audio->setText(tmp);
  }
}

void K3bVcdTrackDialog::prepareGui()
{
  QFrame* frame = plainPage();

  QGridLayout* mainLayout = new QGridLayout( frame );
  mainLayout->setSpacing( spacingHint() );
  mainLayout->setMargin( 0 );

  m_mainTabbed = new QTabWidget( frame );

  ///////////////////////////////////////////////////
  // FILE-INFO BOX
  ///////////////////////////////////////////////////
  QGroupBox* groupFileInfo = new QGroupBox( 0, Qt::Vertical, i18n( "File Info" ), frame, "groupFileInfo" );
  groupFileInfo->layout()->setSpacing( 0 );
  groupFileInfo->layout()->setMargin( 0 );

  QGridLayout* groupFileInfoLayout = new QGridLayout( groupFileInfo->layout() );
  groupFileInfoLayout->setAlignment( Qt::AlignTop );
  groupFileInfoLayout->setSpacing( spacingHint() );
  groupFileInfoLayout->setMargin( marginHint() );

  m_labelMimeType = new QLabel( groupFileInfo, "m_labelMimeType" );

  m_displayFileName = new KCutLabel( groupFileInfo );
  m_displayFileName->setText( i18n( "Filename" ) );
  m_displayFileName->setAlignment( int( QLabel::AlignTop | QLabel::AlignLeft ) );

  QLabel* labelSize = new QLabel( i18n( "Size" ), groupFileInfo, "labelSize" );
  QLabel* labelLength = new QLabel( i18n( "Length"), groupFileInfo, "labelLength" );

  m_displaySize = new QLabel( groupFileInfo, "m_displaySize" );
  m_displaySize->setText( i18n( "0.0 MB" ) );
  m_displaySize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_displayLength = new QLabel( groupFileInfo, "m_displayLength" );
  m_displayLength->setText( i18n( "0:0:0" ) );
  m_displayLength->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  QFrame* fileInfoLine = new QFrame( groupFileInfo );
  fileInfoLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  groupFileInfoLayout->addWidget( m_labelMimeType, 0, 0 );
  groupFileInfoLayout->addMultiCellWidget( m_displayFileName, 0, 1, 1, 2 );
  groupFileInfoLayout->addMultiCellWidget( fileInfoLine, 2, 2, 0, 2 );
  groupFileInfoLayout->addWidget( labelLength, 3, 0 );
  groupFileInfoLayout->addWidget( labelSize, 4, 0 );
  groupFileInfoLayout->addWidget( m_displayLength, 3, 2 );
  groupFileInfoLayout->addWidget( m_displaySize, 4, 2 );

  groupFileInfoLayout->setRowStretch( 5, 1 );

  QFont f( m_displayLength->font() );
  f.setBold( true );
  m_displayLength->setFont( f );
  m_displaySize->setFont( f );
  ///////////////////////////////////////////////////

  mainLayout->addWidget( groupFileInfo, 0, 0 );
  mainLayout->addWidget( m_mainTabbed, 0, 1 );

  mainLayout->setColStretch( 0, 1 );

}

void K3bVcdTrackDialog::setupNavigationTab()
{
  // /////////////////////////////////////////////////
  // NAVIGATION TAB
  // /////////////////////////////////////////////////
  QWidget* w = new QWidget( m_mainTabbed );

  QGridLayout* grid = new QGridLayout( w );
  grid->setAlignment( Qt::AlignTop );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  //////////////////////////////////////////////////////////////////////////////////////////
  QButtonGroup* groupPlay = new QButtonGroup( 4, Qt::Vertical, i18n("Play track ..."), w );
  QGridLayout*  groupPlayLayout = new QGridLayout( groupPlay );
  groupPlayLayout->setAlignment( Qt::AlignTop );
  groupPlayLayout->setSpacing( spacingHint() );
  groupPlayLayout->setMargin( marginHint() );
  
  m_radio_playtime = new QRadioButton( i18n("%1 times").arg(1), groupPlay, "m_radio_playtime" );
  groupPlayLayout->addWidget( m_radio_playtime, 0, 0 );
  
  QSpinBox* spinTimes = new QSpinBox( groupPlay, "m_spinTimes" );
  groupPlayLayout->addWidget( spinTimes, 0, 1 );
  
  m_radio_playforever = new QRadioButton( i18n("forever"), groupPlay, "m_radio_playforever" );
  groupPlayLayout->addWidget( m_radio_playforever, 1, 0 );
  
  //////////////////////////////////////////////////////////////////////////////////////////
  QButtonGroup* groupWait = new QButtonGroup( 4, Qt::Vertical, i18n("than wait ..."), w );
  QGridLayout*  groupWaitLayout = new QGridLayout( groupWait );
  groupWaitLayout->setAlignment( Qt::AlignTop );
  groupWaitLayout->setSpacing( spacingHint() );
  groupWaitLayout->setMargin( marginHint() );
  
  m_radio_waitinfinite = new QRadioButton( i18n("infinite"), groupWait, "m_radio_waitinfinite" );
  groupWaitLayout->addWidget(m_radio_waitinfinite, 0, 0);
  
  m_radio_waittime = new QRadioButton( i18n("%1 seconds").arg(0), groupWait, "m_radio_waittime" );
  groupWaitLayout->addWidget(m_radio_waittime, 1, 0);
  
  QSpinBox* spinSeconds = new QSpinBox( groupWait, "m_spinSeconds" );
  groupWaitLayout->addWidget(spinSeconds, 1, 1);
  
  QLabel* labelAfterTimeout = new QLabel( i18n( "after timeout play" ), groupWait, "labelTimeout" );
  groupWaitLayout->addWidget(labelAfterTimeout, 2, 1);
  
  QComboBox* comboAfterTimeout = new QComboBox( groupWait, "comboAfterTimeout" );
  groupWaitLayout->addWidget(comboAfterTimeout, 3, 1);  
  
  //////////////////////////////////////////////////////////////////////////////////////////
  QGroupBox* groupNav = new QGroupBox( 4, Qt::Vertical, i18n("Navigation"), w );
  QGridLayout*  groupNavLayout = new QGridLayout( groupNav );
  groupNavLayout->setAlignment( Qt::AlignTop );
  groupNavLayout->setSpacing( spacingHint() );
  groupNavLayout->setMargin( marginHint() );
  
  QLabel* labelNav_previous = new QLabel( i18n( "Privious:" ), groupNav, "labelNav_previous" );
  QLabel* labelNav_next  = new QLabel( i18n( "Next:" ), groupNav, "labelNav_next" );
  QLabel* labelNav_return  = new QLabel( i18n( "Return:" ), groupNav, "labelNav_return" );
  QLabel* labelNav_default  = new QLabel( i18n( "Default:" ), groupNav, "labelNav_default" );

  m_nav_previous = new QComboBox( groupNav, "m_nav_previous" );
  m_nav_next = new QComboBox( groupNav, "m_nav_next" );
  m_nav_return = new QComboBox( groupNav, "m_nav_return" );
  m_nav_default = new QComboBox( groupNav, "m_nav_default" );
  
  groupNavLayout->addWidget(labelNav_previous, 0, 0);
  groupNavLayout->addWidget(m_nav_previous, 0, 1);

  groupNavLayout->addWidget(labelNav_next, 1, 0);
  groupNavLayout->addWidget(m_nav_next, 1, 1);

  groupNavLayout->addWidget(labelNav_return, 2, 0);
  groupNavLayout->addWidget(m_nav_return, 2, 1);

  groupNavLayout->addWidget(labelNav_default, 3, 0);
  groupNavLayout->addWidget(m_nav_default, 3, 1);
  
  groupNavLayout->setRowStretch( 4, 1 );
      
  QGroupBox* groupKey = new QGroupBox( 4, Qt::Vertical, i18n("numeric keys"), w );
  m_check_usekeys = new QCheckBox( i18n("Use numeric keys"), groupKey, "m_check_usekeys" );
  m_list_keys = new QListView( groupKey, "m_list_keys" );
  
  grid->addWidget( groupPlay, 0, 0 );
  grid->addWidget( groupWait, 0, 1 );
  grid->addWidget( groupNav, 1, 0 );
  grid->addWidget( groupKey, 1, 1 );
  
  grid->setRowStretch( 2, 1 );

  m_mainTabbed->addTab( w, i18n("Navigation") );  
}

void K3bVcdTrackDialog::setupAudioTab()
{
  // /////////////////////////////////////////////////
  // AUDIO TAB
  // /////////////////////////////////////////////////
  QWidget* w = new QWidget( m_mainTabbed );

  QGridLayout* grid = new QGridLayout( w );
  grid->setAlignment( Qt::AlignTop );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  QLabel* labelMpegVer_Audio = new QLabel( i18n( "Type:" ), w, "labelMpegVer_Audio" );
  QLabel* labelDuration_Audio  = new QLabel( i18n( "Estimated Duration:" ), w, "labelDuration_Audio" );
  QLabel* labelRate_Audio  = new QLabel( i18n( "Rate:" ), w, "labelRate_Audio" );
  QLabel* labelFramesize_Audio  = new QLabel( i18n( "Frame size:" ), w, "labelFramesize_Audio" );
  QLabel* labelMode_Audio  = new QLabel( i18n( "Mode:" ), w, "labelMode_Audio" );
  QLabel* labelExtMode_Audio  = new QLabel( i18n( "Ext. Mode:" ), w, "labelExtMode_Audio" );
  QLabel* labelEmphasis_Audio  = new QLabel( i18n( "Emphasis:" ), w, "labelEmphasis_Audio" );
  QLabel* labelCopyright_Audio  = new QLabel( i18n( "Copyright:" ), w, "labelCopyright_Audio" );

  m_mpegver_audio = new QLabel( w, "m_mpegver_audio" );
  m_duration_audio = new QLabel( w, "m_duration_audio" );
  m_rate_audio = new QLabel( w, "m_rate_audio" );
  m_framesize_audio = new QLabel( w, "m_framesize_audio" );
  m_mode_audio = new QLabel( w, "m_mode_audio" );
  m_extmode_audio = new QLabel( w, "m_extmode_audio" );
  m_emphasis_audio = new QLabel( w, "m_emphasis_audio" );
  m_copyright_audio = new QLabel( w, "m_copyright_audio" );

  grid->addWidget( labelMpegVer_Audio, 1, 0 );
  grid->addWidget( m_mpegver_audio, 1, 1 );
    
  grid->addWidget( labelDuration_Audio, 2, 0 );
  grid->addWidget( m_duration_audio, 2, 1 );
    
  grid->addWidget( labelRate_Audio, 3, 0 );
  grid->addWidget( m_rate_audio, 3, 1 );
    
  grid->addWidget( labelFramesize_Audio, 4, 0 );
  grid->addWidget( m_framesize_audio, 4, 1 );
    
  grid->addWidget( labelMode_Audio, 5, 0 );
  grid->addWidget( m_mode_audio, 5, 1 );
    
  grid->addWidget( labelExtMode_Audio, 6, 0 );
  grid->addWidget( m_extmode_audio, 6, 1 );
    
  grid->addWidget( labelEmphasis_Audio, 7, 0 );
  grid->addWidget( m_emphasis_audio, 7, 1 );  

  grid->addWidget( labelCopyright_Audio, 8, 0 );
  grid->addWidget( m_copyright_audio, 8, 1 );  

  grid->setRowStretch( 9, 1 );

  m_mainTabbed->addTab( w, i18n("Audio") );

}

void K3bVcdTrackDialog::setupVideoTab()
{
  // /////////////////////////////////////////////////
  // VIDEO TAB
  // /////////////////////////////////////////////////
  QWidget* w = new QWidget( m_mainTabbed );

  QGridLayout* grid = new QGridLayout( w );
  grid->setAlignment( Qt::AlignTop );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  QLabel* labelMpegVer_Video = new QLabel( i18n( "Type:" ), w, "labelMpegVer_Video" );
  QLabel* labelDuration_Video  = new QLabel( i18n( "Estimated Duration:" ), w, "labelDuration_Video" );
  QLabel* labelRate_Video  = new QLabel( i18n( "Rate:" ), w, "labelRate_Video" );
  QLabel* labelChromaFormat_Video  = new QLabel( i18n( "Chroma Format:" ), w, "labelChromaFormat_Video" );
  QLabel* labelFormat_Video  = new QLabel( i18n( "Video Format:" ), w, "labelFormat_Video" );
  QLabel* labelSize_Video  = new QLabel( i18n( "Size:" ), w, "labelSize_Video" );
  QLabel* labelDisplaySize_Video  = new QLabel( i18n( "Display Size:" ), w, "labelDisplaySize_Video" );

  m_mpegver_video = new QLabel( w, "m_mpegver_video" );
  m_duration_video = new QLabel( w, "m_duration_video" );
  m_rate_video = new QLabel( w, "m_rate_video" );
  m_chromaformat_video = new QLabel( w, "m_chromaformat_video" );
  m_format_video = new QLabel( w, "m_format_video" );
  m_size_video = new QLabel( w, "m_size_video" );
  m_displaysize_video = new QLabel( w, "m_displaysize_video" );

  grid->addWidget( labelMpegVer_Video, 1, 0 );
  grid->addWidget( m_mpegver_video, 1, 1 );

  grid->addWidget( labelDuration_Video, 2, 0 );
  grid->addWidget( m_duration_video, 2, 1 );

  grid->addWidget( labelRate_Video, 3, 0 );
  grid->addWidget( m_rate_video, 3, 1 );

  grid->addWidget( labelChromaFormat_Video, 4, 0 );
  grid->addWidget( m_chromaformat_video, 4, 1 );

  grid->addWidget( labelFormat_Video, 5, 0 );
  grid->addWidget( m_format_video, 5, 1 );

  grid->addWidget( labelSize_Video, 6, 0 );
  grid->addWidget( m_size_video, 6, 1 );

  grid->addWidget( labelDisplaySize_Video, 7, 0 );
  grid->addWidget( m_displaysize_video, 7, 1 );

  grid->setRowStretch( 9, 1 );
  
  m_mainTabbed->addTab( w, i18n("Video") );
}

#include "k3bvcdtrackdialog.moc"
