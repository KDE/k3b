/***************************************************************************
                          k3bvcdtrackdialog.cpp  -  description
                             -------------------
    begin                : Don Jan 16 2003
    copyright            : (C) 2003 by Sebastian Trueg, Christian Kvasny
    email                : trueg@informatik.uni-freiburg.de
                           chris@ckvsoft.at
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <qlineedit.h>
#include <qmultilineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qhbox.h>
#include <qtabwidget.h>

#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmimetype.h>
#include <kurl.h>

#include "k3bvcdtrackdialog.h"
#include "k3bvcdtrack.h"
#include "../kcutlabel.h"
#include "../tools/k3bglobals.h"


K3bVcdTrackDialog::K3bVcdTrackDialog( QPtrList<K3bVcdTrack>& tracks, QWidget *parent, const char *name )
  : KDialogBase( KDialogBase::Plain, i18n("Video Track Properties"), KDialogBase::Ok,
		 KDialogBase::Ok, parent, name )
{
  setupGui();

  m_tracks = tracks;

  if( !m_tracks.isEmpty() ) {

    K3bVcdTrack* track = m_tracks.first();

    m_displayFileName->setText( track->fileName() );
    m_displayLength->setText( track->mpegDuration() );
    m_displaySize->setText( i18n("%1 kb").arg(track->size() / 1024) );

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
    tmp = "Mpeg 1 System File [Video";
  }
  else {
    tmp.append(i18n("Mpeg 2 Program Stream File [Video"));
  }

  if (track->hasAudio())
    tmp.append(i18n("/Audio]"));
  else
    tmp.append("]");

  m_mpegver_video->setText(tmp);

  m_rate_video->setText(QString("%1 Mbps").arg(track->mpegMbps()));

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
  tmp = track->mpegSize();
  tmp.append(QString(" %1 fps").arg(track->mpegFps()));
  tmp.append(QString(" %1 Mbps").arg(track->mpegMbps()));
  m_size_video->setText(tmp);

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
      case 1: tmp = i18n("Joint Stereo: ");
              if (track->MpegAudioLayer() == 1 || track->MpegAudioLayer() == 2){
                switch (track->MpegAudioModeExt()){
                  case 0: tmp.append(i18n("(Intensity stereo on bands 4-31/32)")); break;
                  case 1: tmp.append(i18n("(Intensity stereo on bands 8-31/32)")); break;
                  case 2: tmp.append(i18n("(Intensity stereo on bands 12-31/32)")); break;
                  case 3: tmp.append(i18n("(Intensity stereo on bands 16-31/32)")); break;
                }
              }
              else {
                //mp3
                switch (track->MpegAudioModeExt()){
                  case 0: tmp.append(i18n("(Intensity stereo off, M/S stereo off)")); break;
                  case 1: tmp.append(i18n("(Intensity stereo on, M/S stereo off)")); break;
                  case 2: tmp.append(i18n("(Intensity stereo off, M/S stereo on)")); break;
                  case 3: tmp.append(i18n("(Intensity stereo on, M/S stereo on)")); break;
                }
              }
              break;
      case 2: tmp = i18n("Dual Channel"); break;
      case 3: tmp = "Mono"; break;
    }
    m_mode_audio->setText(tmp);

    switch (track->MpegAudioEmphasis()){
      case 0: m_emphasis_audio->setText(i18n("No emphasis")); break;
      case 1: m_emphasis_audio->setText(i18n("Emphasis: 50/15 microsecs")); break;
      case 2: m_emphasis_audio->setText(i18n("Emphasis Unknown")); break;
      case 3: m_emphasis_audio->setText(i18n("Emphasis CCITT J 17")); break;
    }

    tmp = "";
    if (track->MpegAudioCopyright()) tmp.append("(c),");
    if (track->MpegAudioOriginal())
      tmp.append(i18n("original"));
    else
      tmp.append(i18n("copy"));

    m_copyright_audio->setText(tmp);
  }
}

void K3bVcdTrackDialog::setupGui()
{
  QFrame* frame = plainPage();

  QGridLayout* mainLayout = new QGridLayout( frame );
  mainLayout->setSpacing( spacingHint() );
  mainLayout->setMargin( 0 );

  QTabWidget* mainTabbed = new QTabWidget( frame );

  // /////////////////////////////////////////////////
  // AUDIO TAB
  // /////////////////////////////////////////////////
  QWidget* audioTab = new QWidget( mainTabbed );

  QGridLayout* audioGrid = new QGridLayout( audioTab );
  audioGrid->setAlignment( Qt::AlignTop );
  audioGrid->setSpacing( spacingHint() );
  audioGrid->setMargin( marginHint() );

  QLabel* labelMpegVer_Audio = new QLabel( i18n( "Type:" ), audioTab, "labelMpegVer_Audio" );
  QLabel* labelDuration_Audio  = new QLabel( i18n( "Estimated Duration:" ), audioTab, "labelDuration_Audio" );
  QLabel* labelRate_Audio  = new QLabel( i18n( "Rate:" ), audioTab, "labelRate_Audio" );
  QLabel* labelFramesize_Audio  = new QLabel( i18n( "Frame size:" ), audioTab, "labelFramesize_Audio" );
  QLabel* labelMode_Audio  = new QLabel( i18n( "Mode:" ), audioTab, "labelMode_Audio" );
  QLabel* labelExtMode_Audio  = new QLabel( i18n( "Ext. Mode:" ), audioTab, "labelExtMode_Audio" );
  QLabel* labelEmphasis_Audio  = new QLabel( i18n( "Emphasis:" ), audioTab, "labelEmphasis_Audio" );
  QLabel* labelCopyright_Audio  = new QLabel( i18n( "Copyright:" ), audioTab, "labelCopyright_Audio" );

  m_mpegver_audio = new QLabel( audioTab, "m_mpegver_audio" );
  m_duration_audio = new QLabel( audioTab, "m_duration_audio" );
  m_rate_audio = new QLabel( audioTab, "m_rate_audio" );
  m_framesize_audio = new QLabel( audioTab, "m_framesize_audio" );
  m_mode_audio = new QLabel( audioTab, "m_mode_audio" );
  m_extmode_audio = new QLabel( audioTab, "m_extmode_audio" );
  m_emphasis_audio = new QLabel( audioTab, "m_emphasis_audio" );
  m_copyright_audio = new QLabel( audioTab, "m_copyright_audio" );

  audioGrid->addWidget( labelMpegVer_Audio, 1, 0 );
  audioGrid->addWidget( m_mpegver_audio, 1, 1 );
    
  audioGrid->addWidget( labelDuration_Audio, 2, 0 );
  audioGrid->addWidget( m_duration_audio, 2, 1 );
    
  audioGrid->addWidget( labelRate_Audio, 3, 0 );
  audioGrid->addWidget( m_rate_audio, 3, 1 );
    
  audioGrid->addWidget( labelFramesize_Audio, 4, 0 );
  audioGrid->addWidget( m_framesize_audio, 4, 1 );
    
  audioGrid->addWidget( labelMode_Audio, 5, 0 );
  audioGrid->addWidget( m_mode_audio, 5, 1 );
    
  audioGrid->addWidget( labelExtMode_Audio, 6, 0 );
  audioGrid->addWidget( m_extmode_audio, 6, 1 );
    
  audioGrid->addWidget( labelEmphasis_Audio, 7, 0 );
  audioGrid->addWidget( m_emphasis_audio, 7, 1 );  

  audioGrid->addWidget( labelCopyright_Audio, 8, 0 );
  audioGrid->addWidget( m_copyright_audio, 8, 1 );  

  audioGrid->setRowStretch( 8, 1 );

  // /////////////////////////////////////////////////
  // VIDEO TAB
  // /////////////////////////////////////////////////
  QWidget* videoTab = new QWidget( mainTabbed );

  QGridLayout* videoGrid = new QGridLayout( videoTab );
  videoGrid->setAlignment( Qt::AlignTop );
  videoGrid->setSpacing( spacingHint() );
  videoGrid->setMargin( marginHint() );

  QLabel* labelMpegVer_Video = new QLabel( i18n( "Type:" ), videoTab, "labelMpegVer_Video" );
  QLabel* labelDuration_Video  = new QLabel( i18n( "Estimated Duration:" ), videoTab, "labelDuration_Video" );
  QLabel* labelRate_Video  = new QLabel( i18n( "Rate:" ), videoTab, "labelRate_Video" );
  QLabel* labelChromaFormat_Video  = new QLabel( i18n( "Chroma Format:" ), videoTab, "labelChromaFormat_Video" );
  QLabel* labelFormat_Video  = new QLabel( i18n( "Video Format:" ), videoTab, "labelFormat_Video" );
  QLabel* labelSize_Video  = new QLabel( i18n( "Size:" ), videoTab, "labelSize_Video" );
  QLabel* labelDisplaySize_Video  = new QLabel( i18n( "Display Size:" ), videoTab, "labelDisplaySize_Video" );

  m_mpegver_video = new QLabel( videoTab, "m_mpegver_video" );
  m_duration_video = new QLabel( videoTab, "m_duration_video" );
  m_rate_video = new QLabel( videoTab, "m_rate_video" );
  m_chromaformat_video = new QLabel( videoTab, "m_chromaformat_video" );
  m_format_video = new QLabel( videoTab, "m_format_video" );
  m_size_video = new QLabel( videoTab, "m_size_video" );
  m_displaysize_video = new QLabel( videoTab, "m_displaysize_video" );

  videoGrid->addWidget( labelMpegVer_Video, 1, 0 );
  videoGrid->addWidget( m_mpegver_video, 1, 1 );

  videoGrid->addWidget( labelDuration_Video, 2, 0 );
  videoGrid->addWidget( m_duration_video, 2, 1 );

  videoGrid->addWidget( labelRate_Video, 3, 0 );
  videoGrid->addWidget( m_rate_video, 3, 1 );

  videoGrid->addWidget( labelChromaFormat_Video, 4, 0 );
  videoGrid->addWidget( m_chromaformat_video, 4, 1 );

  videoGrid->addWidget( labelFormat_Video, 5, 0 );
  videoGrid->addWidget( m_format_video, 5, 1 );

  videoGrid->addWidget( labelSize_Video, 6, 0 );
  videoGrid->addWidget( m_size_video, 6, 1 );

  videoGrid->addWidget( labelDisplaySize_Video, 7, 0 );
  videoGrid->addWidget( m_displaysize_video, 7, 1 );

  videoGrid->setRowStretch( 8, 1 );

  // /////////////////////////////////////////////////
  // FILE-INFO BOX
  // /////////////////////////////////////////////////
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
  // /////////////////////////////////////////////////

  mainTabbed->addTab( videoTab, i18n("Video") );
  mainTabbed->addTab( audioTab, i18n("Audio") );

  mainLayout->addWidget( groupFileInfo, 0, 0 );
  mainLayout->addWidget( mainTabbed, 0, 1 );

  mainLayout->setColStretch( 0, 1 );

}

#include "k3bvcdtrackdialog.moc"
