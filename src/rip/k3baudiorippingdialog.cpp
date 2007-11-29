/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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


#include "k3baudiorippingdialog.h"
#include "k3baudioripjob.h"
#include "k3bpatternparser.h"
#include "k3bcddbpatternwidget.h"
#include "k3baudioconvertingoptionwidget.h"

#include <k3bjobprogressdialog.h>
#include <k3bcore.h>
#include <k3bglobals.h>
#include <k3btrack.h>
#include <k3bstdguiitems.h>
#include <k3bfilesysteminfo.h>
#include <k3bpluginmanager.h>
#include <k3baudioencoder.h>

#include <kcombobox.h>
#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <k3listview.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kio/global.h>
#include <kiconloader.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kurllabel.h>

#include <q3groupbox.h>
#include <q3header.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>

#include <qdir.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qfont.h>

#include <qtoolbutton.h>
#include <qtabwidget.h>
#include <qspinbox.h>
#include <q3ptrlist.h>
#include <q3intdict.h>
#include <qpair.h>
#include <qvalidator.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <Q3ValueList>
#include <kvbox.h>


class K3bAudioRippingDialog::Private
{
public:
  Private() {
  }

  Q3ValueVector<QString> filenames;
  QString playlistFilename;
  K3bFileSystemInfo fsInfo;
};


K3bAudioRippingDialog::K3bAudioRippingDialog(const K3bDevice::Toc& toc, 
					     K3bDevice::Device* device,
					     const K3bCddbResultEntry& entry, 
					     const QList<int>& tracks,
					     QWidget *parent, const char *name )
  : K3bInteractionDialog( parent,
			  QString::null,
			  QString::null,
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  "Audio Ripping" ), // config group
    m_toc( toc ), 
    m_device( device ),
    m_cddbEntry( entry ), 
    m_trackNumbers( tracks )
{
  d = new Private();

  setupGui();
  setupContextHelp();

  K3b::Msf length;
  for( QList<int>::const_iterator it = m_trackNumbers.begin();
       it != m_trackNumbers.end(); ++it ) {
    length += m_toc[*it-1].length();
  }
  setTitle( i18n("CD Ripping"), 
	    i18np("1 track (%1)", "%n tracks (%1)", 
		 m_trackNumbers.count(),length.toString()) );
}


K3bAudioRippingDialog::~K3bAudioRippingDialog()
{
  delete d;
}


void K3bAudioRippingDialog::setupGui()
{
  QWidget *frame = mainWidget();
  Q3GridLayout* Form1Layout = new Q3GridLayout( frame );
  Form1Layout->setSpacing( KDialog::spacingHint() );
  Form1Layout->setMargin( 0 );

  m_viewTracks = new K3ListView( frame );
  m_viewTracks->addColumn(i18n( "Filename") );
  m_viewTracks->addColumn(i18n( "Length") );
  m_viewTracks->addColumn(i18n( "File Size") );
  m_viewTracks->addColumn(i18n( "Type") );
  m_viewTracks->setSorting(-1);
  m_viewTracks->setAllColumnsShowFocus(true);
  m_viewTracks->setFullWidth(true);

  QTabWidget* mainTab = new QTabWidget( frame );

  m_optionWidget = new K3bAudioConvertingOptionWidget( mainTab );
  mainTab->addTab( m_optionWidget, i18n("Settings") );


  // setup filename pattern page
  // -------------------------------------------------------------------------------------------
  m_patternWidget = new K3bCddbPatternWidget( mainTab );
  mainTab->addTab( m_patternWidget, i18n("File Naming") );
  connect( m_patternWidget, SIGNAL(changed()), this, SLOT(refresh()) );


  // setup advanced page
  // -------------------------------------------------------------------------------------------
  QWidget* advancedPage = new QWidget( mainTab );
  Q3GridLayout* advancedPageLayout = new Q3GridLayout( advancedPage );
  advancedPageLayout->setMargin( marginHint() );
  advancedPageLayout->setSpacing( spacingHint() );
  mainTab->addTab( advancedPage, i18n("Advanced") );

  m_comboParanoiaMode = K3bStdGuiItems::paranoiaModeComboBox( advancedPage );
  m_spinRetries = new QSpinBox( advancedPage );
  m_checkIgnoreReadErrors = new QCheckBox( i18n("Ignore read errors"), advancedPage );
  m_checkUseIndex0 = new QCheckBox( i18n("Don't read pregaps"), advancedPage );

  advancedPageLayout->addWidget( new QLabel( i18n("Paranoia mode:"), advancedPage ), 0, 0 );
  advancedPageLayout->addWidget( m_comboParanoiaMode, 0, 1 );
  advancedPageLayout->addWidget( new QLabel( i18n("Read retries:"), advancedPage ), 1, 0 );
  advancedPageLayout->addWidget( m_spinRetries, 1, 1 );
  advancedPageLayout->addMultiCellWidget( m_checkIgnoreReadErrors, 2, 2, 0, 1 );
  advancedPageLayout->addMultiCellWidget( m_checkUseIndex0, 3, 3, 0, 1 );
  advancedPageLayout->setRowStretch( 4, 1 );
  advancedPageLayout->setColStretch( 2, 1 );

  // -------------------------------------------------------------------------------------------


  Form1Layout->addWidget( m_viewTracks, 0, 0 );
  Form1Layout->addWidget( mainTab, 1, 0 );
  Form1Layout->setRowStretch( 0, 1 );

  setStartButtonText( i18n( "Start Ripping" ), i18n( "Starts copying the selected tracks") );
  
  connect( m_checkUseIndex0, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_optionWidget, SIGNAL(changed()), this, SLOT(refresh()) );
}


void K3bAudioRippingDialog::setupContextHelp()
{
  m_spinRetries->setToolTip( i18n("Maximal number of read retries") );
  m_spinRetries->setWhatsThis( i18n("<p>This specifies the maximum number of retries to "
				       "read a sector of audio data from the cd. After that "
				       "K3b will either skip the sector if the <em>Ignore Read Errors</em> "
				       "option is enabled or stop the process.") );
  m_checkUseIndex0->setToolTip( i18n("Do not read the pregaps at the end of every track") );
  m_checkUseIndex0->setWhatsThis( i18n("<p>If this option is checked K3b will not rip the audio "
					  "data in the pregaps. Most audio tracks contain an empty "
					  "pregap which does not belong to the track itself.</p>"
					  "<p>Although the default behaviour of nearly all ripping "
					  "software is to include the pregaps for most CDs it makes more "
					  "sense to ignore them. When creating a K3b audio project you "
					  "will regenerate these pregaps anyway.</p>") );
}


void K3bAudioRippingDialog::init()
{
  refresh();
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
  Q3ListViewItemIterator it( m_viewTracks );
  QStringList filesToOverwrite;
  for( unsigned int i = 0; i < d->filenames.count(); ++i ) {
    if( QFile::exists( d->filenames[i] ) )
      filesToOverwrite.append( d->filenames[i] );
  }

  if( m_optionWidget->createPlaylist() && QFile::exists( d->playlistFilename ) )
    filesToOverwrite.append( d->playlistFilename );

  if( !filesToOverwrite.isEmpty() )
    if( KMessageBox::questionYesNoList( this, 
					i18n("Do you want to overwrite these files?"),
					filesToOverwrite,
					i18n("Files Exist"), KGuiItem(i18n("Overwrite")), KStandardGuiItem::cancel() ) == KMessageBox::No )
      return;


  // prepare list of tracks to rip
  Q3ValueVector<QPair<int, QString> > tracksToRip;
  unsigned int i = 0;
  for( QList<int>::const_iterator trackIt = m_trackNumbers.begin();
       trackIt != m_trackNumbers.end(); ++trackIt ) {
    tracksToRip.append( qMakePair( *trackIt, d->filenames[(m_optionWidget->createSingleFile() ? 0 : i)] ) );
    ++i;
  }

  K3bJobProgressDialog ripDialog( parentWidget(), "Ripping" );

  K3bAudioEncoder* encoder = m_optionWidget->encoder();
  K3bAudioRipJob* job = new K3bAudioRipJob( &ripDialog, this );
  job->setDevice( m_device );
  job->setCddbEntry( m_cddbEntry );
  job->setTracksToRip( tracksToRip );
  job->setParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
  job->setMaxRetries( m_spinRetries->value() );
  job->setNeverSkip( !m_checkIgnoreReadErrors->isChecked() );
  job->setSingleFile( m_optionWidget->createSingleFile() );
  job->setWriteCueFile( m_optionWidget->createCueFile() );
  job->setEncoder( encoder );
  job->setWritePlaylist( m_optionWidget->createPlaylist() );
  job->setPlaylistFilename( d->playlistFilename );
  job->setUseRelativePathInPlaylist( m_optionWidget->playlistRelativePath() );
  job->setUseIndex0( m_checkUseIndex0->isChecked() );
  if( encoder )
    job->setFileType( m_optionWidget->extension() );

  hide();
  ripDialog.startJob(job);

  kDebug() << "(K3bAudioRippingDialog) deleting ripjob.";
  delete job;

  close();
}


void K3bAudioRippingDialog::refresh()
{
  m_viewTracks->clear();
  d->filenames.clear();

  QString baseDir = K3b::prepareDir( m_optionWidget->baseDir() );
  d->fsInfo.setPath( baseDir );

  KIO::filesize_t overallSize = 0;

  if( m_optionWidget->createSingleFile() ) {
    long length = 0;
    for( QList<int>::const_iterator it = m_trackNumbers.begin();
	 it != m_trackNumbers.end(); ++it ) {
      length += ( m_checkUseIndex0->isChecked() 
		  ? m_toc[*it-1].realAudioLength().lba()
		  : m_toc[*it-1].length().lba() );
    }

    QString filename;
    QString extension;
    long long fileSize = 0;
    if( m_optionWidget->encoder() == 0 ) {
      extension = "wav";
      fileSize = length * 2352 + 44;
    }
    else {
      extension = m_optionWidget->extension();
      fileSize = m_optionWidget->encoder()->fileSize( extension, length );
    }

    if( fileSize > 0 )
      overallSize = fileSize;

    if( (int)m_cddbEntry.titles.count() >= 1 ) {
      filename = K3bPatternParser::parsePattern( m_cddbEntry, 1,
						 m_patternWidget->filenamePattern(),
						 m_patternWidget->replaceBlanks(),
						 m_patternWidget->blankReplaceString() );
    }
    else {
      filename = i18n("Album");
    }

    filename = d->fsInfo.fixupPath( filename );

    (void)new K3ListViewItem( m_viewTracks,
			     m_viewTracks->lastItem(),
			     filename + "." + extension,
			     K3b::Msf(length).toString(),
			     fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ),
			     i18n("Audio") );
    d->filenames.append( baseDir + "/" + filename + "." + extension );

    if( m_optionWidget->createCueFile() )
      (void)new K3ListViewItem( m_viewTracks,
			       m_viewTracks->lastItem(),
			       filename + ".cue",
			       "-",
			       "-",
			       i18n("Cue-file") );
  }
  else {
    for( QList<int>::const_iterator it = m_trackNumbers.begin();
	 it != m_trackNumbers.end(); ++it ) {
      int index = *it - 1;

      QString extension;
      long long fileSize = 0;
      K3b::Msf trackLength = ( m_checkUseIndex0->isChecked() 
			       ? m_toc[index].realAudioLength()
			       : m_toc[index].length() );
      if( m_optionWidget->encoder() == 0 ) {
	extension = "wav";
	fileSize = trackLength.audioBytes() + 44;
      }
      else {
	extension = m_optionWidget->extension();
	fileSize = m_optionWidget->encoder()->fileSize( extension, trackLength );
      }

      if( fileSize > 0 )
	overallSize += fileSize;

      if( m_toc[index].type() == K3bTrack::DATA ) {
	extension = ".iso";
	continue;  // TODO: find out how to rip the iso data
      }


      QString filename;

      if( (int)m_cddbEntry.titles.count() >= *it ) {
	filename = K3bPatternParser::parsePattern( m_cddbEntry, *it,
						   m_patternWidget->filenamePattern(),
						   m_patternWidget->replaceBlanks(),
						   m_patternWidget->blankReplaceString() ) + "." + extension;
      }
      else {
	filename = i18n("Track%1", QString::number( *it ).rightJustified( 2, '0' ) ) + "." + extension;
      }

      filename = d->fsInfo.fixupPath( filename );

      (void)new K3ListViewItem( m_viewTracks,
			       m_viewTracks->lastItem(),
			       filename,
			       trackLength.toString(),
			       fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ),
			       (m_toc[index].type() == K3bTrack::AUDIO ? i18n("Audio") : i18n("Data") ) );

      d->filenames.append( baseDir + "/" + filename );
    }
  }

  // create playlist item
  if( m_optionWidget->createPlaylist() ) {
    QString filename = K3bPatternParser::parsePattern( m_cddbEntry, 1,
						       m_patternWidget->playlistPattern(),
						       m_patternWidget->replaceBlanks(),
						       m_patternWidget->blankReplaceString() ) + ".m3u";

    (void)new K3ListViewItem( m_viewTracks,
			     m_viewTracks->lastItem(),
			     filename,
			     "-",
			     "-",
			     i18n("Playlist") );
    
    d->playlistFilename = d->fsInfo.fixupPath( baseDir + "/" + filename );
  }

  if( overallSize > 0 )
    m_optionWidget->setNeededSize( overallSize );
  else
    m_optionWidget->setNeededSize( 0 );
}


void K3bAudioRippingDialog::setStaticDir( const QString& path )
{
  m_optionWidget->setBaseDir( path );
}


void K3bAudioRippingDialog::loadK3bDefaults()
{
  m_comboParanoiaMode->setCurrentItem( 0 );
  m_spinRetries->setValue(5);
  m_checkIgnoreReadErrors->setChecked( true );
  m_checkUseIndex0->setChecked( false );
  
  m_optionWidget->loadDefaults();
  m_patternWidget->loadDefaults();

  refresh();
}

void K3bAudioRippingDialog::loadUserDefaults( const KConfigGroup& c )
{
  m_comboParanoiaMode->setCurrentItem( c.readEntry( "paranoia_mode", 0 ) );
  m_spinRetries->setValue( c.readEntry( "read_retries", 5 ) );
  m_checkIgnoreReadErrors->setChecked( !c.readEntry( "never_skip", true ) );
  m_checkUseIndex0->setChecked( c.readEntry( "use_index0", false ) );

  m_optionWidget->loadConfig( c );
  m_patternWidget->loadConfig( c );

  refresh();
}

void K3bAudioRippingDialog::saveUserDefaults( KConfigGroup& c )
{
  c.writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
  c.writeEntry( "read_retries", m_spinRetries->value() );
  c.writeEntry( "never_skip", !m_checkIgnoreReadErrors->isChecked() );
  c.writeEntry( "use_index0", m_checkUseIndex0->isChecked() );

  m_optionWidget->saveConfig( c );
  m_patternWidget->saveConfig( c );
}


#include "k3baudiorippingdialog.moc"
