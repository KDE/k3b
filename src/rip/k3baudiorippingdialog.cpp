/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bjobprogressdialog.h"
#include "k3bcore.h"
#include "k3bglobals.h"
#include "k3btrack.h"
#include "k3bstdguiitems.h"
#include "k3bfilesysteminfo.h"
#include "k3bpluginmanager.h"
#include "k3baudioencoder.h"
#include "k3bmediacache.h"

#include <KComboBox>
#include <KConfig>
#include <KLocalizedString>
#include <KIO/Global>
#include <KUrlRequester>
#include <KMessageBox>
#include <KUrlLabel>

#include <QDebug>
#include <QDir>
#include <QHash>
#include <QList>
#include <QPair>
#include <QStringList>
#include <QVariant>
#include <QFont>
#include <QValidator>
#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QToolButton>
#include <QToolTip>
#include <QTreeWidget>
#include <QSpinBox>



class K3b::AudioRippingDialog::Private
{
public:
    Private();
    void addTrack( const QString& name, const QString& length, const QString& size, const QString& type );

    QVector<QString> filenames;
    QString playlistFilename;
    K3b::FileSystemInfo fsInfo;

    QTreeWidget* viewTracks;
};


K3b::AudioRippingDialog::Private::Private()
    : viewTracks( 0 )
{
}


void K3b::AudioRippingDialog::Private::addTrack( const QString& name, const QString& length, const QString& size, const QString& type )
{
    QTreeWidgetItem* item = new QTreeWidgetItem( viewTracks );
    item->setText( 0, name );
    item->setText( 1, length );
    item->setText( 2, size );
    item->setText( 3, type );
}


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
    Form1Layout->setContentsMargins( 0, 0, 0, 0 );

    QTreeWidgetItem* header = new QTreeWidgetItem;
    header->setText( 0, i18n( "Filename") );
    header->setText( 1, i18n( "Length") );
    header->setText( 2, i18n( "File Size") );
    header->setText( 3, i18n( "Type") );

    d->viewTracks = new QTreeWidget( frame );
    d->viewTracks->setSortingEnabled( false );
    d->viewTracks->setAllColumnsShowFocus( true );
    d->viewTracks->setHeaderItem( header );
    d->viewTracks->setRootIsDecorated( false );
    d->viewTracks->setSelectionMode( QAbstractItemView::NoSelection );
    d->viewTracks->setFocusPolicy( Qt::NoFocus );
    d->viewTracks->header()->setStretchLastSection( false );
    d->viewTracks->header()->setSectionResizeMode( 0, QHeaderView::Stretch );
    d->viewTracks->header()->setSectionResizeMode( 1, QHeaderView::ResizeToContents );
    d->viewTracks->header()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
    d->viewTracks->header()->setSectionResizeMode( 3, QHeaderView::ResizeToContents );

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
                                         "<p>Although the default behavior of nearly all ripping "
                                         "software is to include the pregaps for most CDs, it makes more "
                                         "sense to ignore them. In any case, when creating a K3b audio "
                                         "project, the pregaps will be regenerated.</p>") );
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
                                            i18n("Files Exist"), KStandardGuiItem::overwrite(), KStandardGuiItem::cancel() ) == KMessageBox::No )
            return;


    // prepare list of tracks to rip
    AudioRipJob::Tracks tracksToRip;
    if( m_optionWidget->createSingleFile() && !d->filenames.isEmpty() ) {
        // Since QMultiMap stores multiple values "from most recently to least recently inserted"
        // we will add it in reverse order to rip in ascending order
        for( int i = m_trackNumbers.count()-1; i >= 0; --i ) {
            tracksToRip.insert( d->filenames.first(), m_trackNumbers[i]+1 );
        }
    }
    else {
        for( int i = 0; i < m_trackNumbers.count() && i < d->filenames.count(); ++i ) {
            tracksToRip.insert( d->filenames[ i ], m_trackNumbers[i]+1 );
        }
    }

    K3b::JobProgressDialog ripDialog( parentWidget(), "Ripping" );

    K3b::AudioEncoder* encoder = m_optionWidget->encoder();
    K3b::AudioRipJob* job = new K3b::AudioRipJob( &ripDialog, this );
    job->setDevice( m_medium.device() );
    job->setCddbEntry( m_cddbEntry );
    job->setTrackList( tracksToRip );
    job->setParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
    job->setMaxRetries( m_spinRetries->value() );
    job->setNeverSkip( !m_checkIgnoreReadErrors->isChecked() );
    job->setEncoder( encoder );
    job->setUseIndex0( m_checkUseIndex0->isChecked() );
    job->setWriteCueFile( m_optionWidget->createSingleFile() && m_optionWidget->createCueFile() );
    if( m_optionWidget->createPlaylist() )
        job->setWritePlaylist( d->playlistFilename, m_optionWidget->playlistRelativePath() );
    if( encoder )
        job->setFileType( m_optionWidget->extension() );

    hide();
    ripDialog.startJob(job);

    qDebug() << "(K3b::AudioRippingDialog) deleting ripjob.";
    delete job;

    close();
}


void K3b::AudioRippingDialog::refresh()
{
    d->viewTracks->clear();
    d->filenames.clear();

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

        filename = d->fsInfo.fixupPath( K3b::PatternParser::parsePattern( m_cddbEntry, 1,
                                                                          extension,
                                                                          m_patternWidget->playlistPattern(),
                                                                          m_patternWidget->replaceBlanks(),
                                                                          m_patternWidget->blankReplaceString() ) );

        d->addTrack( filename,
                     K3b::Msf(length).toString(),
                     fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ),
                     i18n("Audio") );

        d->filenames.append( baseDir + filename );

        if( m_optionWidget->createCueFile() ) {
            QString cueFileName = d->fsInfo.fixupPath( K3b::PatternParser::parsePattern( m_cddbEntry, 1,
                                                                                         QLatin1String( "cue" ),
                                                                                         m_patternWidget->playlistPattern(),
                                                                                         m_patternWidget->replaceBlanks(),
                                                                                         m_patternWidget->blankReplaceString() ) );
            d->addTrack( cueFileName, "-", "-", i18n("Cue-file") );
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
                                                         extension,
                                                         m_patternWidget->filenamePattern(),
                                                         m_patternWidget->replaceBlanks(),
                                                         m_patternWidget->blankReplaceString() );
            if ( filename.isEmpty() ){
                filename = i18n("Track%1", QString::number( trackIndex+1 ).rightJustified( 2, '0' ) ) + '.' + extension;
            }
            filename = d->fsInfo.fixupPath( filename );

            d->addTrack( filename,
                         trackLength.toString(),
                         fileSize < 0 ? i18n("unknown") : KIO::convertSize( fileSize ),
                         toc[trackIndex].type() == K3b::Device::Track::TYPE_AUDIO ? i18n("Audio") : i18n("Data") );

            d->filenames.append( baseDir + filename );
        }
    }

    // create playlist item
    if( m_optionWidget->createPlaylist() ) {
        QString filename = K3b::PatternParser::parsePattern( m_cddbEntry, 1,
                                                             QLatin1String( "m3u" ),
                                                             m_patternWidget->playlistPattern(),
                                                             m_patternWidget->replaceBlanks(),
                                                             m_patternWidget->blankReplaceString() );

        d->addTrack( filename, "-", "-", i18n("Playlist") );

        d->playlistFilename = d->fsInfo.fixupPath( baseDir + '/' + filename );
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


void K3b::AudioRippingDialog::loadSettings( const KConfigGroup& c )
{
    m_comboParanoiaMode->setCurrentIndex( c.readEntry( "paranoia_mode", 0 ) );
    m_spinRetries->setValue( c.readEntry( "read_retries", 5 ) );
    m_checkIgnoreReadErrors->setChecked( !c.readEntry( "never_skip", true ) );
    m_checkUseIndex0->setChecked( c.readEntry( "use_index0", false ) );

    m_optionWidget->loadConfig( c );
    m_patternWidget->loadConfig( c );

    refresh();
}


void K3b::AudioRippingDialog::saveSettings( KConfigGroup c )
{
    c.writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
    c.writeEntry( "read_retries", m_spinRetries->value() );
    c.writeEntry( "never_skip", !m_checkIgnoreReadErrors->isChecked() );
    c.writeEntry( "use_index0", m_checkUseIndex0->isChecked() );

    m_optionWidget->saveConfig( c );
    m_patternWidget->saveConfig( c );
}


