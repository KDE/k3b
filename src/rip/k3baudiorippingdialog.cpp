/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
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
#include "k3bviewcolumnadjuster.h"

#include <k3bjobprogressdialog.h>
#include <k3bcore.h>
#include <k3bglobals.h>
#include <k3btrack.h>
#include <k3bstdguiitems.h>
#include <k3bfilesysteminfo.h>
#include <k3bpluginmanager.h>
#include <k3baudioencoder.h>
#include <k3bmediacache.h>

#include <kcombobox.h>
#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kio/global.h>
#include <kiconloader.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kurllabel.h>

#include <qgroupbox.h>
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
#include <qlist.h>
#include <qhash.h>
#include <qpair.h>
#include <qvalidator.h>
#include <QGridLayout>
#include <QList>
#include <QtGui/QStandardItemModel>
#include <QtGui/QTreeView>
#include <QtGui/QHeaderView>



class K3b::AudioRippingDialog::Private
{
public:
    Private() {
    }

    QVector<QString> filenames;
    QString playlistFilename;
    K3b::FileSystemInfo fsInfo;
    QStandardItemModel trackModel;

    QTreeView* viewTracks;
};


K3b::AudioRippingDialog::AudioRippingDialog( const K3b::Medium& medium,
                                              const KCDDB::CDInfo& entry,
                                              const QList<int>& tracks,
                                              QWidget *parent )
    : K3b::InteractionDialog( parent,
                            QString(),
                            QString(),
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "Audio Ripping" ), // config group
      m_medium( medium ),
      m_cddbEntry( entry ),
      m_trackNumbers( tracks )
{
    d = new Private();

    setupGui();
    setupContextHelp();

    K3b::Msf length;
    K3b::Device::Toc toc = medium.toc();
    for( QList<int>::const_iterator it = m_trackNumbers.constBegin();
         it != m_trackNumbers.constEnd(); ++it ) {
        length += toc[*it].length();
    }
    setTitle( i18n("CD Ripping"),
              i18np("1 track (%2)", "%1 tracks (%2)",
                    m_trackNumbers.count(),length.toString()) );
}


K3b::AudioRippingDialog::~AudioRippingDialog()
{
    delete d;
}


void K3b::AudioRippingDialog::setupGui()
{
    QWidget *frame = mainWidget();
    QGridLayout* Form1Layout = new QGridLayout( frame );
    Form1Layout->setSpacing( KDialog::spacingHint() );
    Form1Layout->setMargin( 0 );

    d->viewTracks = new QTreeView( frame );
    d->viewTracks->setModel( &d->trackModel );
    d->viewTracks->setRootIsDecorated( false );
    K3b::ViewColumnAdjuster* vca = new K3b::ViewColumnAdjuster( d->viewTracks );
    vca->setFixedColumns( QList<int>() << 1 << 2 << 3 );

    QTabWidget* mainTab = new QTabWidget( frame );

    m_optionWidget = new K3b::AudioConvertingOptionWidget( mainTab );
    mainTab->addTab( m_optionWidget, i18n("Settings") );


    // setup filename pattern page
    // -------------------------------------------------------------------------------------------
    m_patternWidget = new K3b::CddbPatternWidget( mainTab );
    mainTab->addTab( m_patternWidget, i18n("File Naming") );
    connect( m_patternWidget, SIGNAL(changed()), this, SLOT(refresh()) );


    // setup advanced page
    // -------------------------------------------------------------------------------------------
    QWidget* advancedPage = new QWidget( mainTab );
    QGridLayout* advancedPageLayout = new QGridLayout( advancedPage );
    advancedPageLayout->setMargin( marginHint() );
    advancedPageLayout->setSpacing( spacingHint() );
    mainTab->addTab( advancedPage, i18n("Advanced") );

    m_comboParanoiaMode = K3b::StdGuiItems::paranoiaModeComboBox( advancedPage );
    m_spinRetries = new QSpinBox( advancedPage );
    m_checkIgnoreReadErrors = new QCheckBox( i18n("Ignore read errors"), advancedPage );
    m_checkUseIndex0 = new QCheckBox( i18n("Do not read pregaps"), advancedPage );

    advancedPageLayout->addWidget( new QLabel( i18n("Paranoia mode:"), advancedPage ), 0, 0 );
    advancedPageLayout->addWidget( m_comboParanoiaMode, 0, 1 );
    advancedPageLayout->addWidget( new QLabel( i18n("Read retries:"), advancedPage ), 1, 0 );
    advancedPageLayout->addWidget( m_spinRetries, 1, 1 );
    advancedPageLayout->addWidget( m_checkIgnoreReadErrors, 2, 0, 0, 1 );
    advancedPageLayout->addWidget( m_checkUseIndex0, 3, 0, 0, 1 );
    advancedPageLayout->setRowStretch( 4, 1 );
    advancedPageLayout->setColumnStretch( 2, 1 );

    // -------------------------------------------------------------------------------------------


    Form1Layout->addWidget( d->viewTracks, 0, 0 );
    Form1Layout->addWidget( mainTab, 1, 0 );
    Form1Layout->setRowStretch( 0, 1 );

    setStartButtonText( i18n( "Start Ripping" ), i18n( "Starts copying the selected tracks") );

    connect( m_checkUseIndex0, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
    connect( m_optionWidget, SIGNAL(changed()), this, SLOT(refresh()) );
}


void K3b::AudioRippingDialog::setupContextHelp()
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


void K3b::AudioRippingDialog::init()
{
    refresh();
}


void K3b::AudioRippingDialog::slotStartClicked()
{
    // check if all filenames differ
    if( d->filenames.count() > 1 ) {
        bool differ = true;
        // the most stupid version to compare but most cds have about 12 tracks
        // that's a size where algorithms do not need any optimization! ;)
        for( int i = 0; i < d->filenames.count(); ++i ) {
            for( int j = i+1; j < d->filenames.count(); ++j )
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
    QStringList filesToOverwrite;
    for( int i = 0; i < d->filenames.count(); ++i ) {
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
    QVector<QPair<int, QString> > tracksToRip;
    unsigned int i = 0;
    for( QList<int>::const_iterator trackIt = m_trackNumbers.constBegin();
         trackIt != m_trackNumbers.constEnd(); ++trackIt ) {
        tracksToRip.append( qMakePair( *trackIt+1, d->filenames[(m_optionWidget->createSingleFile() ? 0 : i)] ) );
        ++i;
    }

    K3b::JobProgressDialog ripDialog( parentWidget(), "Ripping" );

    K3b::AudioEncoder* encoder = m_optionWidget->encoder();
    K3b::AudioRipJob* job = new K3b::AudioRipJob( &ripDialog, this );
    job->setDevice( m_medium.device() );
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

    kDebug() << "(K3b::AudioRippingDialog) deleting ripjob.";
    delete job;

    close();
}


void K3b::AudioRippingDialog::refresh()
{
    d->trackModel.clear();
    d->filenames.clear();

    d->trackModel.setHorizontalHeaderLabels( QStringList() << i18n( "Filename") << i18n( "Length") << i18n( "File Size") << i18n( "Type") );

    QString baseDir = K3b::prepareDir( m_optionWidget->baseDir() );
    d->fsInfo.setPath( baseDir );

    KIO::filesize_t overallSize = 0;

    K3b::Device::Toc toc = m_medium.toc();

    if( m_optionWidget->createSingleFile() ) {
        long length = 0;
        for( QList<int>::const_iterator it = m_trackNumbers.constBegin();
             it != m_trackNumbers.constEnd(); ++it ) {
            length += ( m_checkUseIndex0->isChecked()
                        ? toc[*it].realAudioLength().lba()
                        : toc[*it].length().lba() );
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

        filename = K3b::PatternParser::parsePattern( m_cddbEntry, 1,
                                                   m_patternWidget->filenamePattern(),
                                                   m_patternWidget->replaceBlanks(),
                                                   m_patternWidget->blankReplaceString() );

        filename = d->fsInfo.fixupPath( filename );

        d->trackModel.setItem( 0, 0, new QStandardItem( filename + "." + extension ) );
        d->trackModel.setItem( 0, 1, new QStandardItem( K3b::Msf(length).toString() ) );
        d->trackModel.setItem( 0, 2, new QStandardItem( fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ) ) );
        d->trackModel.setItem( 0, 3, new QStandardItem( i18n("Audio") ) );

        d->filenames.append( baseDir + "/" + filename + "." + extension );

        if( m_optionWidget->createCueFile() ) {
            d->trackModel.setItem( 1, 0, new QStandardItem( filename + ".cue" ) );
            d->trackModel.setItem( 1, 1, new QStandardItem( "-" ) );
            d->trackModel.setItem( 1, 2, new QStandardItem( "-" ) );
            d->trackModel.setItem( 1, 3, new QStandardItem( i18n("Cue-file") ) );
        }
    }
    else {
        for( int i = 0; i < m_trackNumbers.count(); ++i ) {
            int trackIndex = m_trackNumbers[i];

            QString extension;
            long long fileSize = 0;
            K3b::Msf trackLength = ( m_checkUseIndex0->isChecked()
                                     ? toc[trackIndex].realAudioLength()
                                     : toc[trackIndex].length() );
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

            if( toc[trackIndex].type() == K3b::Device::Track::TYPE_DATA ) {
                extension = ".iso";
                continue;  // TODO: find out how to rip the iso data
            }


            QString filename;

            filename = K3b::PatternParser::parsePattern( m_cddbEntry, trackIndex+1,
                                                       m_patternWidget->filenamePattern(),
                                                       m_patternWidget->replaceBlanks(),
                                                       m_patternWidget->blankReplaceString() );
            if ( filename.isEmpty() ){
                filename = i18n("Track%1", QString::number( trackIndex+1 ).rightJustified( 2, '0' ) ) + "." + extension;
            }
            filename = d->fsInfo.fixupPath( filename + "." + extension );

            d->trackModel.setItem( i, 0, new QStandardItem( filename ) );
            d->trackModel.setItem( i, 1, new QStandardItem( trackLength.toString() ) );
            d->trackModel.setItem( i, 2, new QStandardItem( fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ) ) );
            d->trackModel.setItem( i, 3, new QStandardItem( toc[trackIndex].type() == K3b::Device::Track::TYPE_AUDIO ? i18n("Audio") : i18n("Data") ) );

            d->filenames.append( baseDir + "/" + filename );
        }
    }

    // create playlist item
    if( m_optionWidget->createPlaylist() ) {
        QString filename = K3b::PatternParser::parsePattern( m_cddbEntry, 1,
                                                           m_patternWidget->playlistPattern(),
                                                           m_patternWidget->replaceBlanks(),
                                                           m_patternWidget->blankReplaceString() ) + ".m3u";

        int i = d->trackModel.rowCount();
        d->trackModel.setItem( i, 0, new QStandardItem( filename ) );
        d->trackModel.setItem( i, 1, new QStandardItem( "-" ) );
        d->trackModel.setItem( i, 2, new QStandardItem( "-" ) );
        d->trackModel.setItem( i, 3, new QStandardItem( i18n("Playlist") ) );

        d->playlistFilename = d->fsInfo.fixupPath( baseDir + "/" + filename );
    }

    if( overallSize > 0 )
        m_optionWidget->setNeededSize( overallSize );
    else
        m_optionWidget->setNeededSize( 0 );
}


void K3b::AudioRippingDialog::setStaticDir( const QString& path )
{
    m_optionWidget->setBaseDir( path );
}


void K3b::AudioRippingDialog::loadK3bDefaults()
{
    m_comboParanoiaMode->setCurrentIndex( 0 );
    m_spinRetries->setValue(5);
    m_checkIgnoreReadErrors->setChecked( true );
    m_checkUseIndex0->setChecked( false );

    m_optionWidget->loadDefaults();
    m_patternWidget->loadDefaults();

    refresh();
}


void K3b::AudioRippingDialog::loadUserDefaults( const KConfigGroup& c )
{
    m_comboParanoiaMode->setCurrentIndex( c.readEntry( "paranoia_mode", 0 ) );
    m_spinRetries->setValue( c.readEntry( "read_retries", 5 ) );
    m_checkIgnoreReadErrors->setChecked( !c.readEntry( "never_skip", true ) );
    m_checkUseIndex0->setChecked( c.readEntry( "use_index0", false ) );

    m_optionWidget->loadConfig( c );
    m_patternWidget->loadConfig( c );

    refresh();
}


void K3b::AudioRippingDialog::saveUserDefaults( KConfigGroup c )
{
    c.writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
    c.writeEntry( "read_retries", m_spinRetries->value() );
    c.writeEntry( "never_skip", !m_checkIgnoreReadErrors->isChecked() );
    c.writeEntry( "use_index0", m_checkUseIndex0->isChecked() );

    m_optionWidget->saveConfig( c );
    m_patternWidget->saveConfig( c );
}

#include "k3baudiorippingdialog.moc"
