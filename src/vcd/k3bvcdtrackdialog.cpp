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


#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qmultilineedit.h>
#include <qpixmap.h>
#include <qradiobutton.h>
#include <qtable.h>


#include <kiconloader.h>
#include <kio/global.h>
#include <klocale.h>
#include <kmimetype.h>
#include <knuminput.h>
#include <kurl.h>

#include "k3bvcdtrackdialog.h"
#include "k3bvcdtrack.h"
#include "../kcutlabel.h"
#include "../device/k3bmsf.h"
#include "../tools/k3bglobals.h"


K3bVcdTrackDialog::K3bVcdTrackDialog( QPtrList<K3bVcdTrack>& tracks, QPtrList<K3bVcdTrack>& selectedTracks, QWidget *parent, const char *name )
  : KDialogBase( KDialogBase::Plain, i18n("Video Track Properties"), KDialogBase::Ok,
		 KDialogBase::Ok, parent, name )
{
  prepareGui();
  
  setupNavigationTab();
  setupVideoTab();
  setupAudioTab();

  m_tracks = tracks;
  m_selectedTracks = selectedTracks;

  if( !m_selectedTracks.isEmpty() ) {

    K3bVcdTrack* selectedTrack = m_selectedTracks.first();

    m_displayFileName->setText( selectedTrack->fileName() );
    m_displayLength->setText( selectedTrack->mpegDuration() );
    m_displaySize->setText( KIO::convertSize(selectedTrack->size()) );

    m_labelMimeType->setPixmap( KMimeType::pixmapForURL( KURL(selectedTrack->absPath()), 0, KIcon::Desktop, 48 ) );

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

void K3bVcdTrackDialog::slotApply()
{
  // TODO: apply button :)
  K3bVcdTrack* selectedTrack = m_selectedTracks.first();

  if (m_nav_previous->currentItem() > m_tracks.count())
    selectedTrack->setPrevious();
  else
    selectedTrack->setPrevious( m_tracks.at( m_nav_previous->currentItem()) );

  if (m_nav_next->currentItem() > m_tracks.count())
    selectedTrack->setNext();
  else
    selectedTrack->setNext( m_tracks.at( m_nav_next->currentItem()) );

  if (m_nav_return->currentItem() > m_tracks.count())
    selectedTrack->setReturn();
  else
    selectedTrack->setReturn( m_tracks.at( m_nav_return->currentItem()) );
    
  if (m_nav_default->currentItem() > m_tracks.count())
    selectedTrack->setDefault();
  else
    selectedTrack->setDefault( m_tracks.at( m_nav_default->currentItem()) );

}

void K3bVcdTrackDialog::fillGui()
{
  QString tmp;
  K3bVcdTrack* selectedTrack = m_selectedTracks.first();
        
  if (selectedTrack->mpegVideoVersion() == 1) {
    if (selectedTrack->hasAudio())
      m_mpegver_video->setText(i18n("Mpeg 1 System File [Video/Audio]"));
    else
      m_mpegver_video->setText(i18n("Mpeg 1 System File [Video]"));
  }
  else {
    if (selectedTrack->hasAudio())
      m_mpegver_video->setText(i18n("Mpeg Program Stream File [Video/Audio]"));
    else
      m_mpegver_video->setText(i18n("Mpeg Program Stream File [Video]"));
  }

  m_rate_video->setText(i18n("%1 Mbps").arg(selectedTrack->mpegMbps()));

  m_duration_video->setText(selectedTrack->mpegDuration());

  switch (selectedTrack->MpegAspectRatio()) {
    case 0: m_rate_video->setText(i18n("Invalid aspect ratio (forbidden)")); break;
    case 1: m_rate_video->setText(i18n("Aspect ratio 1/1 (VGA)")); break;
    case 2: m_rate_video->setText(i18n("Aspect ratio 4/3 (TV)")); break;
    case 3: m_rate_video->setText(i18n("Aspect ratio 16/9 (large TV)")); break;
    case 4: m_rate_video->setText(i18n("Aspect ratio 2.21/1 (Cinema)")); break;
    default: m_rate_video->setText(i18n("Invalid Aspect ratio (reserved)"));
  }

  m_chromaformat_video->setText(i18n("n/a"));

  if (selectedTrack->MpegSExt()){
    if (selectedTrack->MpegProgressive())
      tmp = i18n("Not interlaced");
    else
      tmp = i18n("Interlaced");

    switch (selectedTrack->MpegChromaFormat()){
      case 1 : tmp.append(", 4:2:0");break;
      case 2 : tmp.append(", 4:2:2");break;
      case 3 : tmp.append(", 4:4:4");break;
    }
    m_chromaformat_video->setText(tmp);
  }

  m_format_video->setText(i18n("n/a"));
  if (selectedTrack->MpegDExt()){
    switch(selectedTrack->MpegFormat()){
      case 0 : m_format_video->setText(i18n("Component"));break;
      case 1 : m_format_video->setText("PAL");break;
      case 2 : m_format_video->setText("NTSC");break;
      case 3 : m_format_video->setText("SECAM");break;
      case 4 : m_format_video->setText("MAC");break;
      case 5 : m_format_video->setText(i18n("Unspecified"));break;
    }
  }

  m_displaysize_video->setText(selectedTrack->mpegDisplaySize());
  m_size_video->setText(i18n("%1 %2 fps %3 Mbps").arg(selectedTrack->mpegSize()).arg(selectedTrack->mpegFps()).arg(selectedTrack->mpegMbps()));


  if (selectedTrack->hasAudio()){
    if (selectedTrack->MpegAudioType() !=3)
      m_mpegver_audio->setText(i18n("Mpeg %1 layer %2").arg(selectedTrack->MpegAudioType()).arg(selectedTrack->MpegAudioLayer()));
    else
      m_mpegver_audio->setText(i18n("Mpeg 2.5 (rare) layer %1").arg(selectedTrack->MpegAudioLayer()));

    if (!selectedTrack->MpegAudioKbps().isNull())
      m_rate_audio->setText(i18n("%1 kbps %2 Hz").arg(selectedTrack->MpegAudioKbps()).arg(selectedTrack->MpegAudioHz()));
    else
      m_rate_audio->setText(i18n("free bitrate %1 Hz").arg(selectedTrack->MpegAudioHz()));

    switch (selectedTrack->MpegAudioMode()){
      case 0: tmp = i18n("Stereo"); break;
      case 1: tmp = i18n("Joint Stereo");
              if (selectedTrack->MpegAudioLayer() == 1 || selectedTrack->MpegAudioLayer() == 2){
                switch (selectedTrack->MpegAudioModeExt()){
                  case 0: tmp.append(" " + i18n("(Intensity stereo on bands 4-31/32)")); break;
                  case 1: tmp.append(" " + i18n("(Intensity stereo on bands 8-31/32)")); break;
                  case 2: tmp.append(" " + i18n("(Intensity stereo on bands 12-31/32)")); break;
                  case 3: tmp.append(" " + i18n("(Intensity stereo on bands 16-31/32)")); break;
                }
              }
              else {
                //mp3
                switch (selectedTrack->MpegAudioModeExt()){
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

    switch (selectedTrack->MpegAudioEmphasis()){
      case 0: m_emphasis_audio->setText(i18n("No emphasis")); break;
      case 1: m_emphasis_audio->setText(i18n("Emphasis 50/15 microsecs")); break;
      case 2: m_emphasis_audio->setText(i18n("Emphasis Unknown")); break;
      case 3: m_emphasis_audio->setText(i18n("Emphasis CCITT J 17")); break;
    }

    tmp = "";
    if (selectedTrack->MpegAudioCopyright()) tmp.append("(c) ");
    if (selectedTrack->MpegAudioOriginal())
      tmp.append(i18n("original"));
    else
      tmp.append(i18n("duplicate"));

    m_copyright_audio->setText(tmp);
  }


  // TODO: make this better :) only for testing now
  /////////////////////////////////////////////////////////////////////////////////////////////////

  // add tracks to combobox
  int iPrevious = -1;
  int iNext = -1;
  int iReturn = -1;
  int iDefault = -1;

  K3bVcdTrack* track;
  for( track = m_tracks.first(); track; track = m_tracks.next() ) {
    QPixmap pm = KMimeType::pixmapForURL( KURL(track->absPath()), 0, KIcon::Desktop, 16 );
    QString s = i18n("%1 - Sequence-%2").arg(track->title()).arg(track->index() +1);
    m_nav_previous->insertItem(pm, s);
    if (track == selectedTrack->Previous())
      iPrevious = m_nav_previous->count() -1;
    
    m_nav_next->insertItem(pm, s);
    if (track == selectedTrack->Next())
      iNext = m_nav_next->count() -1;

    m_nav_return->insertItem(pm, s);
    if (track == selectedTrack->Return())
      iReturn = m_nav_return->count() -1;

    m_nav_default->insertItem(pm, s);
    if (track == selectedTrack->Default())
      iDefault = m_nav_default->count() -1;

    m_comboAfterTimeout->insertItem(pm, s);
  }

  // add Event Disabled
  QPixmap pmDisabled = SmallIcon( "stop" );
  QString txtDisabled = i18n("Event Disabled");
  m_nav_previous->insertItem(pmDisabled, txtDisabled);
  m_nav_next->insertItem(pmDisabled, txtDisabled);
  m_nav_return->insertItem(pmDisabled, txtDisabled);
  m_nav_default->insertItem(pmDisabled, txtDisabled);

  // add VideoCD End
  QPixmap pmEnd = SmallIcon( "cdrom_unmount" );
  QString txtEnd = i18n("VideoCD END");
  m_nav_previous->insertItem(pmEnd, txtEnd);
  m_nav_next->insertItem(pmEnd, txtEnd);
  m_nav_return->insertItem(pmEnd, txtEnd);
  m_nav_default->insertItem(pmEnd, txtEnd);

  if (iPrevious < 0)
    m_nav_previous->setCurrentItem(m_tracks.count());
  else
    m_nav_previous->setCurrentItem(iPrevious);

  if (iNext < 0)
    m_nav_next->setCurrentItem(m_tracks.count());
  else
    m_nav_next->setCurrentItem(iNext);

  if (iReturn < 0)
    m_nav_return->setCurrentItem(m_tracks.count());
  else
    m_nav_return->setCurrentItem(iReturn);

  if (iDefault < 0)
    m_nav_default->setCurrentItem(m_tracks.count());
  else
    m_nav_default->setCurrentItem(iDefault);

  /////////////////////////////////////////////////////////////////////////////////////////////////
  
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
  m_groupPlay = new QButtonGroup(i18n("Playing track ..."), w );
  m_groupPlay->setColumnLayout(0, Qt::Vertical );
  m_groupPlay->setExclusive(true);
  m_groupPlay->layout()->setSpacing( spacingHint() );
  m_groupPlay->layout()->setMargin( marginHint() );

  QGridLayout*  groupPlayLayout = new QGridLayout( m_groupPlay->layout() );
  groupPlayLayout->setAlignment( Qt::AlignTop );
  
  m_spin_times = new QSpinBox( m_groupPlay, "m_spin_times" );
  m_spin_times->setMinValue(1);
  
  m_radio_playtime = new QRadioButton( i18n("%1 times").arg(m_spin_times->value()), m_groupPlay, "m_radio_playtime" );
  m_radio_playtime->setChecked(true);
  m_radio_playforever = new QRadioButton( i18n("forever"), m_groupPlay, "m_radio_playforever" );

  groupPlayLayout->addWidget( m_radio_playtime, 1, 0 );
  groupPlayLayout->addWidget( m_spin_times, 1, 1 );
  groupPlayLayout->addMultiCellWidget( m_radio_playforever, 2, 2, 0, 1 );

  groupPlayLayout->setRowStretch( 5, 1 );
  
  //////////////////////////////////////////////////////////////////////////////////////////
  m_groupWait = new QButtonGroup(i18n("than wait ..."), w );
  m_groupWait->setColumnLayout(0, Qt::Vertical );
  m_groupWait->setExclusive(true);
  m_groupWait->layout()->setSpacing( spacingHint() );
  m_groupWait->layout()->setMargin( marginHint() );
  
  QGridLayout*  groupWaitLayout = new QGridLayout( m_groupWait->layout() );
  groupWaitLayout->setAlignment( Qt::AlignTop );
  
  m_spin_waittime = new QSpinBox( m_groupWait, "m_spinSeconds" );
  m_spin_waittime->setEnabled(false);
  
  m_radio_waitinfinite = new QRadioButton( i18n("infinite"), m_groupWait, "m_radio_waitinfinite" );
  m_radio_waitinfinite->setChecked(true);

  m_radio_waittime = new QRadioButton( i18n("%1 seconds").arg(m_spin_waittime->value()), m_groupWait, "m_radio_waittime" );
  
  QLabel* labelAfterTimeout = new QLabel( i18n( "after timeout playing" ), m_groupWait, "labelTimeout" );

  m_comboAfterTimeout = new QComboBox( m_groupWait, "m_comboAfterTimeout" );
  m_comboAfterTimeout->setEnabled(false);

  groupWaitLayout->addMultiCellWidget(m_radio_waitinfinite, 1, 1, 0, 1);
  groupWaitLayout->addWidget(m_radio_waittime, 2, 0);
  groupWaitLayout->addWidget(m_spin_waittime, 2, 1);
  groupWaitLayout->addMultiCellWidget(labelAfterTimeout, 3, 3, 1, 1);
  groupWaitLayout->addMultiCellWidget(m_comboAfterTimeout, 4, 4, 1, 1);

  groupWaitLayout->setRowStretch( 5, 1 );

  //////////////////////////////////////////////////////////////////////////////////////////
  QGroupBox* groupNav = new QGroupBox(i18n("Key pressed interaction"), w );
  groupNav->setColumnLayout(0, Qt::Vertical );
  groupNav->layout()->setSpacing( spacingHint() );
  groupNav->layout()->setMargin( marginHint() );

  QGridLayout*  groupNavLayout = new QGridLayout( groupNav->layout() );
  groupNavLayout->setAlignment( Qt::AlignTop );
  
  QLabel* labelNav_previous = new QLabel( i18n( "Privious:" ), groupNav, "labelNav_previous" );
  QLabel* labelNav_next  = new QLabel( i18n( "Next:" ), groupNav, "labelNav_next" );
  QLabel* labelNav_return  = new QLabel( i18n( "Return:" ), groupNav, "labelNav_return" );
  QLabel* labelNav_default  = new QLabel( i18n( "Default:" ), groupNav, "labelNav_default" );

  m_nav_previous = new QComboBox( groupNav, "m_nav_previous" );
  m_nav_next = new QComboBox( groupNav, "m_nav_next" );
  m_nav_return = new QComboBox( groupNav, "m_nav_return" );
  m_nav_default = new QComboBox( groupNav, "m_nav_default" );
  groupNavLayout->addWidget(labelNav_previous, 2, 0);
  groupNavLayout->addWidget(m_nav_previous, 2, 1);

  groupNavLayout->addWidget(labelNav_next, 3, 0);
  groupNavLayout->addWidget(m_nav_next, 3, 1);

  groupNavLayout->addWidget(labelNav_return, 4, 0);
  groupNavLayout->addWidget(m_nav_return, 4, 1);

  groupNavLayout->addWidget(labelNav_default, 5, 0);
  groupNavLayout->addWidget(m_nav_default, 5, 1);

  
  //////////////////////////////////////////////////////////////////////////////////////////
  QGroupBox* groupKey = new QGroupBox( 6, Qt::Vertical, i18n("Numeric keys"), w );
  groupKey->setEnabled( false );
  groupKey->layout()->setSpacing( spacingHint() );
  groupKey->layout()->setMargin( marginHint() );

  m_check_usekeys = new QCheckBox( i18n("Use numeric keys"), groupKey, "m_check_usekeys" );
  m_list_keys = new QListView( groupKey, "m_list_keys" );
  m_list_keys->addColumn(i18n("Key"));
  m_list_keys->addColumn(i18n("Playing"));
  m_list_keys->setResizeMode( QListView::LastColumn );
  
  m_check_overwritekeys = new QCheckBox( i18n("Overwrite default assignment"), groupKey, "m_check_overwritekeys" );

  //////////////////////////////////////////////////////////////////////////////////////////
  grid->addWidget( m_groupPlay, 0, 0 );
  grid->addWidget( m_groupWait, 0, 1 );
  grid->addWidget( groupNav, 1, 0 );
  grid->addWidget( groupKey, 1, 1 );

  grid->setRowStretch( 2, 1 );

  m_mainTabbed->addTab( w, i18n("Navigation") );

  connect( m_radio_playforever, SIGNAL(toggled(bool)), this, SLOT(slotPlayForever(bool)) );
  connect( m_radio_waitinfinite, SIGNAL(toggled(bool)), this, SLOT(slotWaitInfinite(bool)) );
  connect( m_spin_times, SIGNAL(valueChanged(int)), this, SLOT(slotPlayTimeChanged(int)) );
  connect( m_spin_waittime, SIGNAL(valueChanged(int)), this, SLOT(slotWaitTimeChanged(int)) );
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

void K3bVcdTrackDialog::slotPlayForever(bool on)
{
  m_groupWait->setDisabled(on);
  m_spin_times->setDisabled(on);
}

void K3bVcdTrackDialog::slotWaitInfinite(bool on)
{
  m_spin_waittime->setDisabled(on);
  m_comboAfterTimeout->setDisabled(on);
}

void K3bVcdTrackDialog::slotPlayTimeChanged(int value)
{
  m_radio_playtime->setText(i18n("%1 times").arg(value));
}

void K3bVcdTrackDialog::slotWaitTimeChanged(int value)
{
  m_radio_waittime->setText(i18n("%1 seconds").arg(value));
}

#include "k3bvcdtrackdialog.moc"
