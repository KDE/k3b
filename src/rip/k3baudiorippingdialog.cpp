/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3baudiorippingdialog.h"
#include "k3bcddacopy.h"
#include "k3bpatternparser.h"
#include <k3bjobprogressdialog.h>
#include "songdb/k3bsong.h"
#include "songdb/k3bsongmanager.h"
#include "../k3b.h"
#include "../tools/k3bglobals.h"
#include "../device/k3btrack.h"
#include "../k3bstdguiitems.h"

#include <kcombobox.h>
#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klistview.h>
#include <klineedit.h>
#include <kfiledialog.h>
#include <kio/global.h>
#include <kiconloader.h>
#include <kstdguiitem.h>
#include <kdebug.h>

#include <qgroupbox.h>
#include <qheader.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qfont.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qhbox.h>
#include <qtoolbutton.h>
#include <qtabwidget.h>
#include <qspinbox.h>




K3bAudioRippingDialog::K3bAudioRippingDialog(const K3bDiskInfo& diskInfo, 
					     const K3bCddbResultEntry& entry, 
					     const QValueList<int>& tracks,
					     QWidget *parent, const char *name )
  : K3bInteractionDialog( parent, name ),
    m_diskInfo( diskInfo ), 
    m_cddbEntry( entry ), 
    m_trackNumbers( tracks )
{
  setupGui();
  setupContextHelp();

  init();

  m_radioMp3->hide(); // not implemented yet
  m_radioOgg->hide(); // not implemented yet

  K3b::Msf length;
  for( QValueList<int>::const_iterator it = m_trackNumbers.begin();
       it != m_trackNumbers.end(); ++it ) {
    length += m_diskInfo.toc[*it].length();
  }
  setTitle( i18n("CD Ripping"), 
	    i18n("1 track (%1)", "%n tracks (%1)", m_trackNumbers.count()).arg(length.toString()) );

  m_radioWav->setChecked(true);
}


void K3bAudioRippingDialog::setupGui()
{
  QWidget *frame = mainWidget();
  QGridLayout* Form1Layout = new QGridLayout( frame );
  Form1Layout->setSpacing( KDialog::spacingHint() );
  Form1Layout->setMargin( 0 );

  m_viewTracks = new KListView( frame, "m_viewTracks" );
  m_viewTracks->addColumn(i18n( "Filename") );
  m_viewTracks->addColumn(i18n( "Length") );
  m_viewTracks->addColumn(i18n( "File Size") );
  m_viewTracks->addColumn(i18n( "Type") );
  m_viewTracks->addColumn(i18n( "Path") );
  m_viewTracks->setSorting(-1);
  m_viewTracks->setAllColumnsShowFocus(true);


  QTabWidget* mainTab = new QTabWidget( frame );

  QWidget* optionPage = new QWidget( mainTab );
  QHBoxLayout* optionPageLayout = new QHBoxLayout( optionPage );
  optionPageLayout->setMargin( marginHint() );
  optionPageLayout->setSpacing( spacingHint() );
  mainTab->addTab( optionPage, i18n("Options") );

  QGroupBox* groupOptions = new QGroupBox( 0, Qt::Vertical, i18n("Destination"), optionPage );
  groupOptions->layout()->setMargin( 0 );
  QGridLayout* groupOptionsLayout = new QGridLayout( groupOptions->layout() );
  groupOptionsLayout->setMargin( marginHint() );
  groupOptionsLayout->setSpacing( spacingHint() );

  QLabel* destLabel = new QLabel( i18n("Destination Base Directory"), groupOptions );
  m_editStaticRipPath = new KLineEdit( groupOptions, "staticeditpattern");
  m_buttonStaticDir = new QToolButton( groupOptions, "m_buttonStaticDir" );
  m_buttonStaticDir->setIconSet( SmallIconSet( "fileopen" ) );
  QFrame* line1 = new QFrame( groupOptions, "line1" );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  m_checkUsePattern = new QCheckBox( i18n("Use directory and filename pattern"), groupOptions );
  m_buttonPattern = new QToolButton( groupOptions, "m_buttonPattern" );
  m_buttonPattern->setIconSet( SmallIconSet( "gear" ) );

  groupOptionsLayout->addMultiCellWidget( destLabel, 0, 0, 0, 1 );
  groupOptionsLayout->addWidget( m_editStaticRipPath, 1, 0 );
  groupOptionsLayout->addWidget( m_buttonStaticDir, 1, 1 );
  groupOptionsLayout->addMultiCellWidget( line1, 2, 2, 0, 1 );
  groupOptionsLayout->addWidget( m_checkUsePattern, 3, 0 );
  groupOptionsLayout->addWidget( m_buttonPattern, 3, 1 );
  groupOptionsLayout->setColStretch( 0, 1 );


  m_groupFileType = new QButtonGroup( 4, Qt::Vertical, i18n("File Type"), optionPage );
  m_radioWav = new QRadioButton( i18n("Wave"), m_groupFileType );
  m_radioMp3 = new QRadioButton( i18n("MP3"), m_groupFileType );
  m_radioOgg = new QRadioButton( i18n("Ogg Vorbis"), m_groupFileType ); // TODO: test if ogg available

  optionPageLayout->addWidget( groupOptions );
  optionPageLayout->addWidget( m_groupFileType );
  optionPageLayout->setStretchFactor( groupOptions, 1 );


  // setup advanced page
  // -------------------------------------------------------------------------------------------
  QWidget* advancedPage = new QWidget( mainTab );
  QGridLayout* advancedPageLayout = new QGridLayout( advancedPage );
  advancedPageLayout->setMargin( marginHint() );
  advancedPageLayout->setSpacing( spacingHint() );
  mainTab->addTab( advancedPage, i18n("Advanced") );

  QGroupBox* groupReading = new QGroupBox( 3, Qt::Vertical, i18n("Reading options"), advancedPage );
  groupReading->setInsideSpacing( spacingHint() );
  groupReading->setInsideMargin( marginHint() );

  QHBox* paranoiaBox = new QHBox( groupReading );
  (void)new QLabel( i18n("Paranoia mode:"), paranoiaBox );
  m_comboParanoiaMode = K3bStdGuiItems::paranoiaModeComboBox( paranoiaBox );
  QHBox* retriesBox = new QHBox( groupReading );
  (void)new QLabel( i18n("Read retries:"), retriesBox );
  m_spinRetries = new QSpinBox( retriesBox );
  m_checkNeverSkip = new QCheckBox( i18n("Never skip"), groupReading );


  QGroupBox* groupMisc = new QGroupBox( 1, Qt::Vertical, i18n("Misc. options"), advancedPage );
  groupMisc->setInsideSpacing( spacingHint() );
  groupMisc->setInsideMargin( marginHint() );
  m_checkSingleFile = new QCheckBox( i18n("Create single file"), groupMisc );

  advancedPageLayout->addWidget( groupReading, 0, 0 );
  advancedPageLayout->addWidget( groupMisc, 0, 1 );
  advancedPageLayout->setColStretch( 1, 1 );
  // -------------------------------------------------------------------------------------------


  Form1Layout->addWidget( m_viewTracks, 0, 0 );
  Form1Layout->addWidget( mainTab, 1, 0 );
  Form1Layout->setRowStretch( 0, 1 );

  setStartButtonText( i18n( "Start Ripping" ), i18n( "Starts copying the selected tracks") );

  connect( m_buttonPattern, SIGNAL(clicked() ), this, SLOT(showPatternDialog()) );
  connect( m_buttonStaticDir, SIGNAL(clicked()), this, SLOT(slotFindStaticDir()) );
  connect( m_checkUsePattern, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_radioWav, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_radioOgg, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_radioMp3, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
}


void K3bAudioRippingDialog::setupContextHelp()
{
  QToolTip::add( m_spinRetries, i18n("Maximal number of read retries") );
  QWhatsThis::add( m_spinRetries, i18n("<p>This specifies the maximum number of retries to "
				       "read a sector of audio data from the cd. After that "
				       "K3b will either skip the sector or stop the process "
				       "if the <em>never-skip</em> option is enabled.") );
  QToolTip::add( m_checkNeverSkip, i18n("Never skip a sector on error") );
  QWhatsThis::add( m_checkNeverSkip, i18n("<p>If this option is checked K3b will never skip "
					  "an audio sector if it was not readable (see retries)."
					  "<p>K3b will stop the ripping process if a read error "
					  "occurs.") );
  QToolTip::add( m_checkSingleFile, i18n("Rip all tracks to a single file") );
  QWhatsThis::add( m_checkSingleFile, i18n("<p>If this option is checked K3b will create only one "
					   "audio file no matter how many tracks are ripped. This "
					   "file will contain all tracks one after the other."
					   "<p>This might be useful to rip a live album or a radio play."
					   "<p><b>Caution:</b> The file will have the name of the first track.") ); 
}


K3bAudioRippingDialog::~K3bAudioRippingDialog()
{
}


void K3bAudioRippingDialog::init()
{
  slotLoadUserDefaults();
  refresh();
}

void K3bAudioRippingDialog::slotStartClicked()
{
  KConfig* c = kapp->config();
  c->setGroup( "Ripping" );

  c->writeEntry( "last ripping directory", m_editStaticRipPath->text() );
  QString filetype;
  if( m_radioOgg->isChecked() )
    filetype = "ogg";
  else if( m_radioMp3->isChecked() )
    filetype = "mp3";
  else
    filetype = "wav";

  c->writeEntry( "last used filetype", filetype );

//   // save all entries with artist title, path filename and so on
//   setSongList();


  K3bCddaCopy* job = new K3bCddaCopy( this );
  job->setDevice( m_diskInfo.device );

  job->setCddbEntry( m_cddbEntry );
  job->setUsePattern( m_checkUsePattern->isChecked() );
  job->setBaseDirectory( m_editStaticRipPath->text() );
  job->setCopyTracks( m_trackNumbers );
  job->setParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
  job->setMaxRetries( m_spinRetries->value() );
  job->setNeverSkip( m_checkNeverSkip->isChecked() );
  job->setSingleFile( m_checkSingleFile->isChecked() );

  K3bJobProgressDialog ripDialog( this, "Ripping" );
  ripDialog.setJob( job );

  job->start();
  hide();
  ripDialog.exec();

  delete job;

  close();
}


void K3bAudioRippingDialog::showPatternDialog()
{
  k3bMain()->showOptionDialog( 4 );
  refresh();
}



void K3bAudioRippingDialog::refresh()
{
  KConfig* c = kapp->config();
  c->setGroup( "Audio Ripping" );

  m_viewTracks->clear();


  for( QValueList<int>::const_iterator it = m_trackNumbers.begin();
       it != m_trackNumbers.end(); ++it ) {
    int index = *it - 1;

    QString extension;
    if( m_radioOgg->isChecked() )
      extension = ".ogg";
    else if( m_radioMp3->isChecked() )
      extension = ".mp3";
    else
      extension = ".wav";

    long fileSize = m_diskInfo.toc[index].length() * 2352;

    if( m_diskInfo.toc[index].type() == K3bTrack::DATA ) {
      extension = ".iso";
      continue;  // TODO: find out how to rip the iso data
    }
    else {
      // FIXME: mp3,ogg file size recalculation
    }


    QString filename, directory;

    if( m_checkUsePattern->isChecked() && (int)m_cddbEntry.titles.count() >= *it ) {
      filename = K3bPatternParser::parsePattern( m_cddbEntry, *it,
						 c->readEntry( "filename pattern", "%a - %t" ),
						 c->readBoolEntry( "replace blank in filename", false ),
						 c->readEntry( "filename replace string", "_" ) ) + extension;

      directory = K3bPatternParser::parsePattern( m_cddbEntry, *it,
						  c->readEntry( "directory pattern", "%r/%m" ),
						  c->readBoolEntry( "replace blank in directory", false ),
						  c->readEntry( "directory replace string", "_" ) );
    }
    else {
      filename = i18n("Track%1").arg( QString::number( *it ).rightJustify( 2, '0' ) ) + extension;
      directory = "";
    }

    (void)new KListViewItem( m_viewTracks,
			     m_viewTracks->lastItem(),
			     filename,
			     K3b::Msf(m_diskInfo.toc[index].length()).toString(),
			     KIO::convertSize( fileSize ),
			     (m_diskInfo.toc[index].type() == K3bTrack::AUDIO ? i18n("Audio") : i18n("Data") ),
			     directory );
  }
}


void K3bAudioRippingDialog::slotFindStaticDir() {
  QString path = KFileDialog::getExistingDirectory( m_editStaticRipPath->text(), this, i18n("Select Ripping Directory") );
  if( !path.isEmpty() ) {
    m_editStaticRipPath->setText( path );
  }
}

void K3bAudioRippingDialog::setStaticDir( const QString& path ){
    m_editStaticRipPath->setText( path );
}


void K3bAudioRippingDialog::slotLoadK3bDefaults()
{
  // set reasonable defaults
  m_editStaticRipPath->setText( QDir::homeDirPath() );
  m_checkUsePattern->setChecked( true );

  m_comboParanoiaMode->setCurrentItem( 3 );
  m_spinRetries->setValue(20);
  m_checkNeverSkip->setChecked( false );
  m_checkSingleFile->setChecked( false );
}

void K3bAudioRippingDialog::slotLoadUserDefaults()
{
  KConfig* c = k3bMain()->config();
  c->setGroup( "Audio Ripping" );

  m_editStaticRipPath->setText( c->readEntry( "last ripping directory", QDir::homeDirPath() ) );
  m_checkUsePattern->setChecked( c->readBoolEntry( "use_pattern", true ) );

  m_comboParanoiaMode->setCurrentItem( c->readNumEntry( "paranoia_mode", 3 ) );
  m_spinRetries->setValue( c->readNumEntry( "read_retries", 20 ) );
  m_checkNeverSkip->setChecked( c->readBoolEntry( "never_skip", false ) );
  m_checkSingleFile->setChecked( c->readBoolEntry( "single_file", false ) );
}

void K3bAudioRippingDialog::slotSaveUserDefaults()
{
  KConfig* c = k3bMain()->config();
  c->setGroup( "Audio Ripping" );

  c->writeEntry( "last ripping directory", m_editStaticRipPath->text() );
  c->writeEntry( "use_pattern", m_checkUsePattern->isChecked() );

  c->writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
  c->writeEntry( "read_retries", m_spinRetries->value() );
  c->writeEntry( "never_skip", m_checkNeverSkip->isChecked() );
  c->writeEntry( "single_file", m_checkSingleFile->isChecked() );
}

#include "k3baudiorippingdialog.moc"
