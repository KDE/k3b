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


#include "k3baudiorippingdialog.h"
#include "k3baudioripthread.h"
#include "k3bpatternparser.h"
#include "k3bcddbpatternwidget.h"
#include "base_k3baudiorippingoptionwidget.h"

#include <k3bjobprogressdialog.h>
#include "songdb/k3bsong.h"
#include "songdb/k3bsongmanager.h"
#include <k3bcore.h>
#include <k3bglobals.h>
#include <k3btrack.h>
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
#include <kurllabel.h>

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
#include <qvalidator.h>


class K3bAudioRippingDialog::Private
{
public:
  Private() {
  }

  QIntDict<K3bAudioEncoderFactory> factoryMap;
  QValueVector<QString> filenames;
  QMap<int, QString> extensionMap;
  QString playlistFilename;
};


K3bAudioRippingDialog::K3bAudioRippingDialog(const K3bCdDevice::Toc& toc, 
					     K3bCdDevice::CdDevice* device,
					     const K3bCddbResultEntry& entry, 
					     const QValueList<int>& tracks,
					     QWidget *parent, const char *name )
  : K3bInteractionDialog( parent, name ),
    m_toc( toc ), 
    m_device( device ),
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
    length += m_toc[*it-1].length();
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
  m_viewTracks->addColumn(i18n( "Filename (relative to base directory)") );
  m_viewTracks->addColumn(i18n( "Length") );
  m_viewTracks->addColumn(i18n( "File Size") );
  m_viewTracks->addColumn(i18n( "Type") );
  m_viewTracks->setSorting(-1);
  m_viewTracks->setAllColumnsShowFocus(true);
  m_viewTracks->setFullWidth(true);

  QTabWidget* mainTab = new QTabWidget( frame );

  m_optionWidget = new base_K3bAudioRippingOptionWidget( mainTab );
  mainTab->addTab( m_optionWidget, i18n("Options") );

  m_optionWidget->m_buttonConfigurePlugin->setIconSet( SmallIconSet( "gear" ) );


  // setup filename pattern page
  // -------------------------------------------------------------------------------------------
  m_patternWidget = new K3bCddbPatternWidget( mainTab );
  mainTab->addTab( m_patternWidget, i18n("File Naming") );
  connect( m_patternWidget, SIGNAL(changed()), this, SLOT(refresh()) );

  connect( m_optionWidget->m_checkUsePattern, SIGNAL(toggled(bool)), m_patternWidget, SLOT(setEnabled(bool)) );


  // setup advanced page
  // -------------------------------------------------------------------------------------------
  QWidget* advancedPage = new QWidget( mainTab );
  QGridLayout* advancedPageLayout = new QGridLayout( advancedPage );
  advancedPageLayout->setMargin( marginHint() );
  advancedPageLayout->setSpacing( spacingHint() );
  mainTab->addTab( advancedPage, i18n("Advanced") );

  m_comboParanoiaMode = K3bStdGuiItems::paranoiaModeComboBox( advancedPage );
  m_spinRetries = new QSpinBox( advancedPage );
  m_checkNeverSkip = new QCheckBox( i18n("Never skip"), advancedPage );
  m_checkUseIndex0 = new QCheckBox( i18n("Don't read pregaps"), advancedPage );

  advancedPageLayout->addWidget( new QLabel( i18n("Paranoia mode:"), advancedPage ), 0, 0 );
  advancedPageLayout->addWidget( m_comboParanoiaMode, 0, 1 );
  advancedPageLayout->addWidget( new QLabel( i18n("Read retries:"), advancedPage ), 1, 0 );
  advancedPageLayout->addWidget( m_spinRetries, 1, 1 );
  advancedPageLayout->addMultiCellWidget( m_checkNeverSkip, 2, 2, 0, 1 );
  advancedPageLayout->addMultiCellWidget( m_checkUseIndex0, 3, 3, 0, 1 );
  advancedPageLayout->setRowStretch( 4, 1 );
  advancedPageLayout->setColStretch( 2, 1 );

  // -------------------------------------------------------------------------------------------


  Form1Layout->addWidget( m_viewTracks, 0, 0 );
  Form1Layout->addWidget( mainTab, 1, 0 );
  Form1Layout->setRowStretch( 0, 1 );

  setStartButtonText( i18n( "Start Ripping" ), i18n( "Starts copying the selected tracks") );
  
  connect( m_optionWidget->m_checkUsePattern, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_optionWidget->m_checkCreatePlaylist, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_optionWidget->m_checkSingleFile, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_checkUseIndex0, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_optionWidget->m_comboFileType, SIGNAL(activated(int)), this, SLOT(refresh()) );
  connect( m_optionWidget->m_editBaseDir, SIGNAL(textChanged(const QString&)), this, SLOT(refresh()) );
  connect( m_optionWidget->m_buttonConfigurePlugin, SIGNAL(clicked()), this, SLOT(slotConfigurePlugin()) );
  connect( m_optionWidget->m_comboFileType, SIGNAL(activated(int)), this, SLOT(slotToggleAll()) );

  connect( m_patternWidget->m_specialStringsLabel, SIGNAL(leftClickedURL()), this, SLOT(slotSeeSpecialStrings()) );
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
  QToolTip::add( m_checkUseIndex0, i18n("Do not read the pregaps at the end of every track") );
  QWhatsThis::add( m_checkUseIndex0, i18n("<p>If this option is checked K3b will not rip the audio "
					  "data in the pregaps. Most audio tracks contain an empty "
					  "pregap which does not belong to the track itself.</p>"
					  "<p>Although the default behaviour of nearly all ripping "
					  "software is to include the pregaps for most CDs it makes more "
					  "sense to ignore them. When creating a K3b audio project you "
					  "will regenerate these pregaps anyway.</p>") );
}


void K3bAudioRippingDialog::init()
{
  d->factoryMap.clear();
  d->extensionMap.clear();
  m_optionWidget->m_comboFileType->clear();
  m_optionWidget->m_comboFileType->insertItem( i18n("Wave") );

  // check the available encoding plugins
  QPtrList<K3bPluginFactory> fl = k3bpluginmanager->factories( "AudioEncoder" );
  for( QPtrListIterator<K3bPluginFactory> it( fl ); it.current(); ++it ) {
    K3bAudioEncoderFactory* f = (K3bAudioEncoderFactory*)it.current();
    QStringList exL = f->extensions();

    for( QStringList::const_iterator exIt = exL.begin();
	 exIt != exL.end(); ++exIt ) {
      d->extensionMap.insert( m_optionWidget->m_comboFileType->count(), *exIt );
      d->factoryMap.insert( m_optionWidget->m_comboFileType->count(), f );
      m_optionWidget->m_comboFileType->insertItem( f->fileTypeComment(*exIt) );
    }
  }

  slotLoadUserDefaults();
  refresh();

  m_patternWidget->setEnabled( m_optionWidget->m_checkUsePattern->isChecked() );
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

  if( QFile::exists( d->playlistFilename ) )
    filesToOverwrite.append( d->playlistFilename );

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
    tracksToRip.append( qMakePair( *trackIt, d->filenames[(m_optionWidget->m_checkSingleFile->isChecked() ? 0 : i)] ) );
    ++i;
  }


  K3bAudioEncoderFactory* factory = d->factoryMap[m_optionWidget->m_comboFileType->currentItem()];  // 0 for wave

  K3bAudioRipThread* thread = new K3bAudioRipThread();
  thread->setDevice( m_device );
  thread->setCddbEntry( m_cddbEntry );
  thread->setTracksToRip( tracksToRip );
  thread->setParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
  thread->setMaxRetries( m_spinRetries->value() );
  thread->setNeverSkip( m_checkNeverSkip->isChecked() );
  thread->setSingleFile( m_optionWidget->m_checkSingleFile->isChecked() );
  thread->setEncoderFactory( factory );
  thread->setWritePlaylist( m_optionWidget->m_checkCreatePlaylist->isChecked() );
  thread->setPlaylistFilename( d->playlistFilename );
  thread->setUseRelativePathInPlaylist( m_optionWidget->m_checkPlaylistRelative->isChecked() );
  thread->setUseIndex0( m_checkUseIndex0->isChecked() );
  if( factory )
    thread->setFileType( d->extensionMap[m_optionWidget->m_comboFileType->currentItem()] );

  K3bJobProgressDialog ripDialog( kapp->mainWidget(), "Ripping" );

  K3bThreadJob job( thread, &ripDialog, this );

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

  QString baseDir = K3b::prepareDir( m_optionWidget->m_editBaseDir->url() );

  if( m_optionWidget->m_checkSingleFile->isChecked() ) {
    long length = 0;
    for( QValueList<int>::const_iterator it = m_trackNumbers.begin();
	 it != m_trackNumbers.end(); ++it ) {
      length += ( m_checkUseIndex0->isChecked() 
		  ? m_toc[*it-1].realAudioLength().lba()
		  : m_toc[*it-1].length().lba() );
    }

    QString filename;
    QString extension;
    long long fileSize = 0;
    if( m_optionWidget->m_comboFileType->currentItem() == 0 ) {
      extension = "wav";
      fileSize = length * 2352 + 44;
    }
    else {
      extension = d->extensionMap[m_optionWidget->m_comboFileType->currentItem()];
      fileSize = d->factoryMap[m_optionWidget->m_comboFileType->currentItem()]->fileSize( extension, length );
    }

    if( m_optionWidget->m_checkUsePattern->isChecked() && (int)m_cddbEntry.titles.count() >= 1 ) {
      filename = K3bPatternParser::parsePattern( m_cddbEntry, 1,
						 m_patternWidget->filenamePattern(),
						 m_patternWidget->replaceBlanks(),
						 m_patternWidget->blankReplaceString() ) + "." + extension;
    }
    else {
      filename = i18n("Album") + "." + extension;
    }


    (void)new KListViewItem( m_viewTracks,
			     m_viewTracks->lastItem(),
			     filename,
			     K3b::Msf(length).toString(),
			     fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ),
			     i18n("Audio") );

    d->filenames.append( K3b::fixupPath( baseDir + "/" + filename ) );
  }
  else {
    for( QValueList<int>::const_iterator it = m_trackNumbers.begin();
	 it != m_trackNumbers.end(); ++it ) {
      int index = *it - 1;

      QString extension;
      long long fileSize = 0;
      K3b::Msf trackLength = ( m_checkUseIndex0->isChecked() 
			       ? m_toc[index].realAudioLength()
			       : m_toc[index].length() );
      if( m_optionWidget->m_comboFileType->currentItem() == 0 ) {
	extension = "wav";
	fileSize = trackLength.audioBytes() + 44;
      }
      else {
	extension = d->extensionMap[m_optionWidget->m_comboFileType->currentItem()];
	fileSize = d->factoryMap[m_optionWidget->m_comboFileType->currentItem()]->fileSize( extension, trackLength );
      }

      if( m_toc[index].type() == K3bTrack::DATA ) {
	extension = ".iso";
	continue;  // TODO: find out how to rip the iso data
      }


      QString filename;

      if( m_optionWidget->m_checkUsePattern->isChecked() && (int)m_cddbEntry.titles.count() >= *it ) {
	filename = K3bPatternParser::parsePattern( m_cddbEntry, *it,
						   m_patternWidget->filenamePattern(),
						   m_patternWidget->replaceBlanks(),
						   m_patternWidget->blankReplaceString() ) + "." + extension;
      }
      else {
	filename = i18n("Track%1").arg( QString::number( *it ).rightJustify( 2, '0' ) ) + "." + extension;
      }

      (void)new KListViewItem( m_viewTracks,
			       m_viewTracks->lastItem(),
			       filename,
			       trackLength.toString(),
			       fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ),
			       (m_toc[index].type() == K3bTrack::AUDIO ? i18n("Audio") : i18n("Data") ) );

      d->filenames.append( K3b::fixupPath( baseDir + "/" + filename ) );
    }
  }

  // create playlist item
  if( m_optionWidget->m_checkCreatePlaylist->isChecked() ) {
    QString filename = K3bPatternParser::parsePattern( m_cddbEntry, 1,
						       m_patternWidget->playlistPattern(),
						       m_patternWidget->replaceBlanks(),
						       m_patternWidget->blankReplaceString() );

    (void)new KListViewItem( m_viewTracks,
			     m_viewTracks->lastItem(),
			     filename,
			     "-",
			     "-",
			     i18n("Playlist") );
    
    d->playlistFilename = K3b::fixupPath( baseDir + "/" + filename );
  }
}


void K3bAudioRippingDialog::setStaticDir( const QString& path )
{
  m_optionWidget->m_editBaseDir->setURL( path );
}


void K3bAudioRippingDialog::slotLoadK3bDefaults()
{
  // set reasonable defaults
  m_optionWidget->m_editBaseDir->setURL( QDir::homeDirPath() );
  m_optionWidget->m_checkUsePattern->setChecked( true );

  m_comboParanoiaMode->setCurrentItem( 3 );
  m_spinRetries->setValue(20);
  m_checkNeverSkip->setChecked( false );
  m_checkUseIndex0->setChecked( false );
  
  m_optionWidget->m_checkSingleFile->setChecked( false );

  m_optionWidget->m_comboFileType->setCurrentItem(0); // Wave

  m_patternWidget->loadDefaults();

  m_optionWidget->m_checkCreatePlaylist->setChecked(false);
  m_optionWidget->m_checkPlaylistRelative->setChecked(false);

  slotToggleAll();
}

void K3bAudioRippingDialog::slotLoadUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "Audio Ripping" );

  m_optionWidget->m_editBaseDir->setURL( c->readPathEntry( "last ripping directory", QDir::homeDirPath() ) );
  m_optionWidget->m_checkUsePattern->setChecked( c->readBoolEntry( "use_pattern", true ) );

  m_comboParanoiaMode->setCurrentItem( c->readNumEntry( "paranoia_mode", 3 ) );
  m_spinRetries->setValue( c->readNumEntry( "read_retries", 20 ) );
  m_checkNeverSkip->setChecked( c->readBoolEntry( "never_skip", false ) );
  m_checkUseIndex0->setChecked( c->readBoolEntry( "use_index0", false ) );

  m_optionWidget->m_checkSingleFile->setChecked( c->readBoolEntry( "single_file", false ) );

  m_optionWidget->m_checkCreatePlaylist->setChecked( c->readBoolEntry( "create_playlist", false ) );
  m_optionWidget->m_checkPlaylistRelative->setChecked( c->readBoolEntry( "relative_path_in_playlist", false ) );

  QString filetype = c->readEntry( "filetype", "wav" );
  if( filetype == "wav" )
    m_optionWidget->m_comboFileType->setCurrentItem(0);
  else {
    for( QMap<int, QString>::iterator it = d->extensionMap.begin();
	 it != d->extensionMap.end(); ++it ) {
      if( it.data() == filetype ) {
	m_optionWidget->m_comboFileType->setCurrentItem( it.key() );
	break;
      }
    }
  }
  m_patternWidget->loadConfig( c );

  slotToggleAll();
}

void K3bAudioRippingDialog::slotSaveUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "Audio Ripping" );

  c->writePathEntry( "last ripping directory", m_optionWidget->m_editBaseDir->url() );
  c->writeEntry( "use_pattern", m_optionWidget->m_checkUsePattern->isChecked() );

  c->writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
  c->writeEntry( "read_retries", m_spinRetries->value() );
  c->writeEntry( "never_skip", m_checkNeverSkip->isChecked() );
  c->writeEntry( "use_index0", m_checkUseIndex0->isChecked() );

  c->writeEntry( "single_file", m_optionWidget->m_checkSingleFile->isChecked() );

  c->writeEntry( "create_playlist", m_optionWidget->m_checkCreatePlaylist->isChecked() );
  c->writeEntry( "relative_path_in_playlist", m_optionWidget->m_checkPlaylistRelative->isChecked() );

  if( d->extensionMap.contains(m_optionWidget->m_comboFileType->currentItem()) )
    c->writeEntry( "filetype", d->extensionMap[m_optionWidget->m_comboFileType->currentItem()] );
  else
    c->writeEntry( "filetype", "wav" );

  m_patternWidget->saveConfig( c );
}


void K3bAudioRippingDialog::slotConfigurePlugin()
{
  K3bAudioEncoderFactory* factory = d->factoryMap[m_optionWidget->m_comboFileType->currentItem()];  // 0 for wave
  if( factory )
    k3bpluginmanager->execPluginDialog( factory, this );
}


void K3bAudioRippingDialog::slotToggleAll()
{
  m_optionWidget->m_buttonConfigurePlugin->setEnabled( d->factoryMap[m_optionWidget->m_comboFileType->currentItem()] != 0 );  // 0 for wave
}


void K3bAudioRippingDialog::slotSeeSpecialStrings()
{
  QWhatsThis::display( i18n( "<p><b>Pattern special strings:</b>"
			     "<ul>\n"
			     "<li>%a - artist of the track\n"
			     "<li>%t - title of the track\n"
			     "<li>%n - track number\n"
			     "<li>%y - year of the CD\n"
			     "<li>%e - extended information about the track\n"
			     "<li>%g - genre of the CD\n"
			     "<li>%r - album artist (differs from %a only on soundtracks or compilations)\n"
			     "<li>%m - album title\n"
			     "<li>%x - extended information about the CD\n"
			     "<li>%d - current date\n"
			     "</ul>" ) );
}

#include "k3baudiorippingdialog.moc"
