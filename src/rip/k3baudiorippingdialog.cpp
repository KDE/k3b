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
#include "k3baudioripthread.h"
#include "k3bpatternparser.h"
#include "k3bcddbpatternwidget.h"
#include <k3bjobprogressdialog.h>
#include "songdb/k3bsong.h"
#include "songdb/k3bsongmanager.h"
#include <k3bcore.h>
#include <k3bglobals.h>
#include <device/k3btrack.h>
#include <k3bstdguiitems.h>
#include <k3bthreadjob.h>

#include <k3bpluginmanager.h>
#include <k3baudioencoder.h>

#include <kcombobox.h>
#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klistview.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kio/global.h>
#include <kiconloader.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <kmessagebox.h>

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
#include <qhbox.h>
#include <qtoolbutton.h>
#include <qtabwidget.h>
#include <qspinbox.h>
#include <qptrlist.h>
#include <qintdict.h>
#include <qpair.h>



class K3bAudioRippingDialog::Private
{
public:
  Private() {
  }

  QIntDict<K3bAudioEncoderFactory> factoryMap;
  QValueVector<QString> filenames;
  QMap<int, QString> extensionMap;
};


K3bAudioRippingDialog::K3bAudioRippingDialog(const K3bDiskInfo& diskInfo, 
					     const K3bCddbResultEntry& entry, 
					     const QValueList<int>& tracks,
					     QWidget *parent, const char *name )
  : K3bInteractionDialog( parent, name ),
    m_diskInfo( diskInfo ), 
    m_cddbEntry( entry ), 
    m_trackNumbers( tracks )
{
  d = new Private();

  setupGui();
  setupContextHelp();

  init();

  K3b::Msf length;
  for( QValueList<int>::const_iterator it = m_trackNumbers.begin();
       it != m_trackNumbers.end(); ++it ) {
    length += m_diskInfo.toc[*it-1].length();
  }
  setTitle( i18n("CD Ripping"), 
	    i18n("1 track (%1)", "%n tracks (%1)", 
		 m_trackNumbers.count()).arg(length.toString()) );
}


K3bAudioRippingDialog::~K3bAudioRippingDialog()
{
  delete d;
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
  m_editStaticRipPath = new KURLRequester( groupOptions, "staticeditpattern");
  m_editStaticRipPath->setMode( KFile::Directory );
  QFrame* line1 = new QFrame( groupOptions, "line1" );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  m_checkUsePattern = new QCheckBox( i18n("Use directory and filename pattern"), groupOptions );

  groupOptionsLayout->addMultiCellWidget( destLabel, 0, 0, 0, 1 );
  groupOptionsLayout->addMultiCellWidget( m_editStaticRipPath, 1, 1, 0, 1 );
  groupOptionsLayout->addMultiCellWidget( line1, 2, 2, 0, 1 );
  groupOptionsLayout->addMultiCellWidget( m_checkUsePattern, 3, 3, 0, 1 );
  groupOptionsLayout->setRowStretch( 4, 1 );


  QGroupBox* groupFileType = new QGroupBox( 1, Qt::Vertical, i18n("File Type"), optionPage );
  m_comboFileType = new QComboBox( groupFileType );

  // TODO: add a button for configuring the plugins

  optionPageLayout->addWidget( groupOptions );
  optionPageLayout->addWidget( groupFileType );
  optionPageLayout->setStretchFactor( groupOptions, 1 );


  // setup filename pattern page
  // -------------------------------------------------------------------------------------------
  m_patternWidget = new K3bCddbPatternWidget( mainTab );
  mainTab->addTab( m_patternWidget, i18n("File Naming") );
  connect( m_patternWidget, SIGNAL(changed()), this, SLOT(refresh()) );

  connect( m_checkUsePattern, SIGNAL(toggled(bool)), m_patternWidget, SLOT(setEnabled(bool)) );

  // setup advanced page
  // -------------------------------------------------------------------------------------------
  QWidget* advancedPage = new QWidget( mainTab );
  QGridLayout* advancedPageLayout = new QGridLayout( advancedPage );
  advancedPageLayout->setMargin( marginHint() );
  advancedPageLayout->setSpacing( spacingHint() );
  mainTab->addTab( advancedPage, i18n("Advanced") );

  QGroupBox* groupReading = new QGroupBox( 3, Qt::Vertical, i18n("Reading Options"), advancedPage );
  groupReading->setInsideSpacing( spacingHint() );
  groupReading->setInsideMargin( marginHint() );

  QHBox* paranoiaBox = new QHBox( groupReading );
  (void)new QLabel( i18n("Paranoia mode:"), paranoiaBox );
  m_comboParanoiaMode = K3bStdGuiItems::paranoiaModeComboBox( paranoiaBox );
  QHBox* retriesBox = new QHBox( groupReading );
  (void)new QLabel( i18n("Read retries:"), retriesBox );
  m_spinRetries = new QSpinBox( retriesBox );
  m_checkNeverSkip = new QCheckBox( i18n("Never skip"), groupReading );


  QGroupBox* groupMisc = new QGroupBox( 1, Qt::Vertical, i18n("Misc. Options"), advancedPage );
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

  connect( m_checkUsePattern, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_checkSingleFile, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_comboFileType, SIGNAL(activated(int)), this, SLOT(refresh()) );
  connect( m_editStaticRipPath, SIGNAL(textChanged(const QString&)), this, SLOT(refresh()) );
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


void K3bAudioRippingDialog::init()
{
  d->factoryMap.clear();
  d->extensionMap.clear();
  m_comboFileType->clear();
  m_comboFileType->insertItem( i18n("Wave") );

  // check the available encoding plugins
  QPtrList<K3bPluginFactory> fl = k3bpluginmanager->factories( "AudioEncoder" );
  for( QPtrListIterator<K3bPluginFactory> it( fl ); it.current(); ++it ) {
    K3bAudioEncoderFactory* f = (K3bAudioEncoderFactory*)it.current();
    QStringList exL = f->extensions();

    for( QStringList::const_iterator exIt = exL.begin();
	 exIt != exL.end(); ++exIt ) {
      d->extensionMap.insert( m_comboFileType->count(), *exIt );
      d->factoryMap.insert( m_comboFileType->count(), f );
      m_comboFileType->insertItem( f->fileTypeComment(*exIt) );
    }
  }

  slotLoadUserDefaults();
  refresh();

  m_patternWidget->setEnabled( m_checkUsePattern->isChecked() );
}


void K3bAudioRippingDialog::slotStartClicked()
{
  // check if all filenames differ
  if( d->filenames.count() > 1 ) {
    bool differ = true;
    // the most stupid version to compare but most cds have about 12 tracks
    // that's a size where algorithms do not need any optimization! ;)
    for( unsigned int i = 0; i < d->filenames.count(); ++i ) {
      for( unsigned int j = i+1; j < d->filenames.count(); ++j )
	if( d->filenames[i] == d->filenames[j] ) {
	  differ = false;
	  break;
	}
    }

    if( !differ ) {
      KMessageBox::sorry( this, i18n("Please check the naming pattern. All filenames need to be unique.") );
      return;
    }
  }

  // check if we need to overwrite some files...
  QListViewItemIterator it( m_viewTracks );
  QStringList filesToOverwrite;
  for( unsigned int i = 0; i < d->filenames.count(); ++i ) {
    if( QFile::exists( d->filenames[i] ) )
      filesToOverwrite.append( d->filenames[i] );
  }

  if( !filesToOverwrite.isEmpty() )
    if( KMessageBox::questionYesNoList( this, 
					i18n("Do you want to overwrite these files?"),
					filesToOverwrite,
					i18n("Files exist") ) == KMessageBox::No )
      return;


  // prepare list of tracks to rip
  QValueVector<QPair<int, QString> > tracksToRip;
  unsigned int i = 0;
  for( QValueList<int>::const_iterator trackIt = m_trackNumbers.begin();
       trackIt != m_trackNumbers.end(); ++trackIt ) {
    tracksToRip.append( qMakePair( *trackIt, d->filenames[(m_checkSingleFile->isChecked() ? 0 : i)] ) );
    ++i;
  }


  K3bAudioEncoderFactory* factory = d->factoryMap[m_comboFileType->currentItem()];  // 0 for wave

  K3bAudioRipThread* thread = new K3bAudioRipThread();
  thread->setDevice( m_diskInfo.device );
  thread->setCddbEntry( m_cddbEntry );
  thread->setUsePattern( m_checkUsePattern->isChecked() );
  thread->setBaseDirectory( m_editStaticRipPath->url() );
  thread->setDirectoryPattern( m_patternWidget->directoryPattern() );
  thread->setFilenamePattern( m_patternWidget->filenamePattern() );
  thread->setDirectoryReplaceString( m_patternWidget->directoryReplaceString() );
  thread->setFilenameReplaceString( m_patternWidget->filenameReplaceString() );
  thread->setReplaceBlanksInDir( m_patternWidget->replaceBlanksInDirectory() );
  thread->setReplaceBlanksInFilename( m_patternWidget->replaceBlanksInFilename() );
  thread->setTracksToRip( tracksToRip );
  thread->setParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
  thread->setMaxRetries( m_spinRetries->value() );
  thread->setNeverSkip( m_checkNeverSkip->isChecked() );
  thread->setSingleFile( m_checkSingleFile->isChecked() );
  thread->setEncoderFactory( factory );
  if( factory )
    thread->setFileType( d->extensionMap[m_comboFileType->currentItem()] );
  K3bThreadJob job( thread, this );

  K3bJobProgressDialog ripDialog( kapp->mainWidget(), "Ripping" );

  hide();
  ripDialog.startJob(&job);

  delete thread;

  close();
}


void K3bAudioRippingDialog::refresh()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "Audio Ripping" );

  m_viewTracks->clear();
  d->filenames.clear();

  QString baseDir = m_editStaticRipPath->url();
  if( baseDir[baseDir.length()-1] != '/' )
    baseDir += "/";

  if( m_checkSingleFile->isChecked() ) {
    long length = 0;
    for( QValueList<int>::const_iterator it = m_trackNumbers.begin();
	 it != m_trackNumbers.end(); ++it ) {
      length += m_diskInfo.toc[*it-1].length().lba();
    }

    QString filename, directory;
    QString extension;
    long long fileSize = 0;
    if( m_comboFileType->currentItem() == 0 ) {
      extension = "wav";
      fileSize = length * 2352 + 44;
    }
    else {
      extension = d->extensionMap[m_comboFileType->currentItem()];
      fileSize = d->factoryMap[m_comboFileType->currentItem()]->fileSize( extension, length );
    }

    if( m_checkUsePattern->isChecked() && (int)m_cddbEntry.titles.count() >= 1 ) {
      filename = K3bPatternParser::parsePattern( m_cddbEntry, 1,
						 m_patternWidget->filenamePattern(),
						 m_patternWidget->replaceBlanksInFilename(),
						 m_patternWidget->filenameReplaceString() ) + "." + extension;

      directory = K3bPatternParser::parsePattern( m_cddbEntry, 1,
						  m_patternWidget->directoryPattern(),
						  m_patternWidget->replaceBlanksInDirectory(),
						  m_patternWidget->directoryReplaceString() );
    }
    else {
      filename = i18n("Album") + "." + extension;
      directory = "";
    }


    (void)new KListViewItem( m_viewTracks,
			     m_viewTracks->lastItem(),
			     filename,
			     K3b::Msf(length).toString(),
			     fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ),
			     i18n("Audio"),
			     directory );

    d->filenames.append( baseDir + directory + "/" + filename );
  }
  else {
    for( QValueList<int>::const_iterator it = m_trackNumbers.begin();
	 it != m_trackNumbers.end(); ++it ) {
      int index = *it - 1;

      QString extension;
      long long fileSize = 0;
      if( m_comboFileType->currentItem() == 0 ) {
	extension = "wav";
	fileSize = m_diskInfo.toc[index].length().audioBytes() + 44;
      }
      else {
	extension = d->extensionMap[m_comboFileType->currentItem()];
	fileSize = d->factoryMap[m_comboFileType->currentItem()]->fileSize( extension, m_diskInfo.toc[index].length() );
      }

      if( m_diskInfo.toc[index].type() == K3bTrack::DATA ) {
	extension = ".iso";
	continue;  // TODO: find out how to rip the iso data
      }


      QString filename, directory;

      if( m_checkUsePattern->isChecked() && (int)m_cddbEntry.titles.count() >= *it ) {
	filename = K3bPatternParser::parsePattern( m_cddbEntry, *it,
						   m_patternWidget->filenamePattern(),
						   m_patternWidget->replaceBlanksInFilename(),
						   m_patternWidget->filenameReplaceString() ) + "." + extension;

	directory = K3bPatternParser::parsePattern( m_cddbEntry, *it,
						    m_patternWidget->directoryPattern(),
						    m_patternWidget->replaceBlanksInDirectory(),
						    m_patternWidget->directoryReplaceString() );
      }
      else {
	filename = i18n("Track%1").arg( QString::number( *it ).rightJustify( 2, '0' ) ) + "." + extension;
	directory = "";
      }

      (void)new KListViewItem( m_viewTracks,
			       m_viewTracks->lastItem(),
			       filename,
			       K3b::Msf(m_diskInfo.toc[index].length()).toString(),
			       fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ),
			       (m_diskInfo.toc[index].type() == K3bTrack::AUDIO ? i18n("Audio") : i18n("Data") ),
			       directory );

      d->filenames.append( baseDir + directory + "/" + filename );
    }
  }
}


void K3bAudioRippingDialog::setStaticDir( const QString& path ){
  m_editStaticRipPath->setURL( path );
}


void K3bAudioRippingDialog::slotLoadK3bDefaults()
{
  // set reasonable defaults
  m_editStaticRipPath->setURL( QDir::homeDirPath() );
  m_checkUsePattern->setChecked( true );

  m_comboParanoiaMode->setCurrentItem( 3 );
  m_spinRetries->setValue(20);
  m_checkNeverSkip->setChecked( false );
  m_checkSingleFile->setChecked( false );

  m_comboFileType->setCurrentItem(0); // Wave

  m_patternWidget->loadDefaults();
}

void K3bAudioRippingDialog::slotLoadUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "Audio Ripping" );

  m_editStaticRipPath->setURL( c->readPathEntry( "last ripping directory", QDir::homeDirPath() ) );
  m_checkUsePattern->setChecked( c->readBoolEntry( "use_pattern", true ) );

  m_comboParanoiaMode->setCurrentItem( c->readNumEntry( "paranoia_mode", 3 ) );
  m_spinRetries->setValue( c->readNumEntry( "read_retries", 20 ) );
  m_checkNeverSkip->setChecked( c->readBoolEntry( "never_skip", false ) );
  m_checkSingleFile->setChecked( c->readBoolEntry( "single_file", false ) );

  QString filetype = c->readEntry( "filetype", "wav" );
  if( filetype == "wav" )
    m_comboFileType->setCurrentItem(0);
  else {
    for( QMap<int, QString>::iterator it = d->extensionMap.begin();
	 it != d->extensionMap.end(); ++it ) {
      if( it.data() == filetype ) {
	m_comboFileType->setCurrentItem( it.key() );
	break;
      }
    }
  }
  m_patternWidget->loadConfig( c );
}

void K3bAudioRippingDialog::slotSaveUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "Audio Ripping" );

  c->writeEntry( "last ripping directory", m_editStaticRipPath->url() );
  c->writeEntry( "use_pattern", m_checkUsePattern->isChecked() );

  c->writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
  c->writeEntry( "read_retries", m_spinRetries->value() );
  c->writeEntry( "never_skip", m_checkNeverSkip->isChecked() );
  c->writeEntry( "single_file", m_checkSingleFile->isChecked() );

  if( d->extensionMap.contains(m_comboFileType->currentItem()) )
    c->writeEntry( "filetype", d->extensionMap[m_comboFileType->currentItem()] );
  else
    c->writeEntry( "filetype", "wav" );

  m_patternWidget->saveConfig( c );
}

#include "k3baudiorippingdialog.moc"
