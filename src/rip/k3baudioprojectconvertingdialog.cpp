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


#include "k3baudioprojectconvertingdialog.h"
#include "k3baudioprojectconvertingjob.h"
#include "k3bpatternparser.h"
#include "k3bcddbpatternwidget.h"
#include "k3baudioconvertingoptionwidget.h"

#include "k3baudiodoc.h"
#include "k3baudioview.h"
#include "k3baudiotrack.h"
#include "k3bjobprogressdialog.h"
#include "k3bcore.h"
#include "k3bglobals.h"
#include "k3baudioencoder.h"

#include <KComboBox>
#include <KConfig>
#include <KLocalizedString>
#include <KIO/Global>
#include <KUrlRequester>
#include <KMessageBox>

#include <QDebug>
#include <QDir>
#include <QStringList>
#include <QCheckBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QTabWidget>
#include <QTreeWidget>

#include <KCddb/Cdinfo>


namespace {
    KCDDB::CDInfo createCddbEntryFromDoc( K3b::AudioDoc* doc )
    {
        KCDDB::CDInfo e;

        // global
        e.set( KCDDB::Title, doc->title() );
        e.set( KCDDB::Artist, doc->artist() );
        e.set( KCDDB::Comment, doc->cdTextMessage() );

        // tracks
        int i = 0;
        K3b::AudioTrack* track = doc->firstTrack();
        while( track ) {
            e.track( i ).set( KCDDB::Title, track->title() );
            e.track( i ).set( KCDDB::Artist, track->artist() );
            e.track( i ).set( KCDDB::Comment, track->cdTextMessage() );

            track = track->next();
            ++i;
        }

        return e;
    }
}

class K3b::AudioProjectConvertingDialog::Private
{
public:
    Private() {
    }

    QTreeWidget* viewTracks;
    QVector<QString> filenames;
    QString playlistFilename;
};


K3b::AudioProjectConvertingDialog::AudioProjectConvertingDialog( K3b::AudioDoc* doc, QWidget *parent )
    : K3b::InteractionDialog( parent,
                            QString(),
                            QString(),
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "Audio Project Converting" ), // config group
      m_doc(doc)
{
    d = new Private();

    setupGui();

    setTitle( i18n("Audio Project Conversion"),
              i18np("1 track (%2)", "%1 tracks (%2)",
                    m_doc->numOfTracks(),m_doc->length().toString()) );

    refresh();
}


K3b::AudioProjectConvertingDialog::~AudioProjectConvertingDialog()
{
    delete d;
}


void K3b::AudioProjectConvertingDialog::setupGui()
{
    QWidget *frame = mainWidget();
    QGridLayout* Form1Layout = new QGridLayout( frame );
    Form1Layout->setContentsMargins( 0, 0, 0, 0 );

    QTreeWidgetItem* header = new QTreeWidgetItem;
    header->setText( 0, i18n( "Filename (relative to base folder)") );
    header->setText( 1, i18n( "Length") );
    header->setText( 2, i18n( "File Size") );

    d->viewTracks = new QTreeWidget( frame );
    d->viewTracks->setSortingEnabled( false );
    d->viewTracks->setAllColumnsShowFocus( true );
    d->viewTracks->setRootIsDecorated( false );
    d->viewTracks->setSelectionMode( QAbstractItemView::NoSelection );
    d->viewTracks->setFocusPolicy( Qt::NoFocus );
    d->viewTracks->setHeaderItem( header );
    d->viewTracks->header()->setStretchLastSection( false );
    d->viewTracks->header()->setSectionResizeMode( 0, QHeaderView::Stretch );
    d->viewTracks->header()->setSectionResizeMode( 1, QHeaderView::ResizeToContents );
    d->viewTracks->header()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );

    QTabWidget* mainTab = new QTabWidget( frame );

    m_optionWidget = new K3b::AudioConvertingOptionWidget( mainTab );
    mainTab->addTab( m_optionWidget, i18n("Settings") );


    // setup filename pattern page
    // -------------------------------------------------------------------------------------------
    m_patternWidget = new K3b::CddbPatternWidget( mainTab );
    mainTab->addTab( m_patternWidget, i18n("File Naming") );
    connect( m_patternWidget, SIGNAL(changed()), this, SLOT(refresh()) );

    Form1Layout->addWidget( d->viewTracks, 0, 0 );
    Form1Layout->addWidget( mainTab, 1, 0 );
    Form1Layout->setRowStretch( 0, 1 );

    connect( m_optionWidget, SIGNAL(changed()), this, SLOT(refresh()) );
}


void K3b::AudioProjectConvertingDialog::slotStartClicked()
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
        if( KMessageBox::warningContinueCancelList( this,
                                                    i18n("Do you want to overwrite these files?"),
                                                    filesToOverwrite,
                                                    i18n("Files Exist"), KStandardGuiItem::overwrite() )
            == KMessageBox::Cancel )
            return;


    // just generate a fake m_tracks list for now so we can keep most of the methods
    // like they are in K3b::AudioRipJob. This way future combination is easier
    AudioProjectConvertingJob::Tracks tracksToRip;
    if( m_optionWidget->createSingleFile() && !d->filenames.isEmpty() ) {
        // Since QMultiMap stores multiple values "from most recently to least recently inserted"
        // we will add it in reverse order to rip in ascending order
        for( AudioTrack* track = m_doc->lastTrack(); track != 0; track = track->prev() )
            tracksToRip.insert( d->filenames.front(), track->trackNumber() );
    }
    else {
        for( AudioTrack* track = m_doc->firstTrack(); track != 0; track = track->next() )
            tracksToRip.insert( d->filenames[ track->trackNumber()-1 ], track->trackNumber() );
    }

    K3b::AudioEncoder* encoder = m_optionWidget->encoder();

    K3b::JobProgressDialog progressDialog( parentWidget() );
    K3b::AudioProjectConvertingJob job( m_doc, &progressDialog, 0 );
    job.setCddbEntry( createCddbEntryFromDoc( m_doc ) );
    job.setTrackList( tracksToRip );
    job.setEncoder( encoder );
    job.setWriteCueFile( m_optionWidget->createSingleFile() && m_optionWidget->createCueFile() );
    if( m_optionWidget->createPlaylist() )
        job.setWritePlaylist( d->playlistFilename, m_optionWidget->playlistRelativePath() );
    if( encoder )
        job.setFileType( m_optionWidget->extension() );

    hide();
    progressDialog.startJob(&job);

    close();
}


void K3b::AudioProjectConvertingDialog::refresh()
{
#ifdef __GNUC__
#warning Reuse the code from AudioRippingDialog
#endif
    d->viewTracks->clear();
    d->filenames.clear();

    // FIXME: this is bad and needs to be improved
    // create a cddb entry from the doc to use in the patternparser
    KCDDB::CDInfo cddbEntry = createCddbEntryFromDoc( m_doc );

    QString baseDir = K3b::prepareDir( m_optionWidget->baseDir() );

    QString extension = m_optionWidget->extension();

    KIO::filesize_t overallSize = 0;

    if( m_optionWidget->createSingleFile() ) {
        QString filename;
        long long filesize = 0;
        if( m_optionWidget->encoder() == 0 ) {
            filesize = m_doc->length().audioBytes() + 44;
        }
        else {
            filesize = m_optionWidget->encoder()->fileSize( extension, m_doc->length() );
        }

        if( filesize > 0 )
            overallSize = filesize;

        filename = K3b::PatternParser::parsePattern( cddbEntry, 1,
                                                     extension,
                                                     m_patternWidget->playlistPattern(),
                                                     m_patternWidget->replaceBlanks(),
                                                     m_patternWidget->blankReplaceString() );

        QTreeWidgetItem* item = new QTreeWidgetItem( d->viewTracks );
        item->setText( 0, filename );
        item->setText( 1, m_doc->length().toString() );
        item->setText( 2, filesize < 0 ? i18n("unknown") : KIO::convertSize( filesize ) );

        d->filenames.append( K3b::fixupPath( baseDir + '/' + filename ) );

        if( m_optionWidget->createCueFile() ) {
            QString cueFilename = K3b::PatternParser::parsePattern( cddbEntry, 1,
                                                               QLatin1String( "cue" ),
                                                               m_patternWidget->playlistPattern(),
                                                               m_patternWidget->replaceBlanks(),
                                                               m_patternWidget->blankReplaceString() );
            item = new QTreeWidgetItem( d->viewTracks );
            item->setText( 0, cueFilename );
            item->setText( 1, "-" );
            item->setText( 2, "-" );
        }
    }
    else {
        d->filenames.resize( m_doc->numOfTracks() );
        for( AudioTrack* track = m_doc->firstTrack(); track != 0; track = track->next() ) {
            long long filesize = 0;
            if( m_optionWidget->encoder() == 0 ) {
                filesize = track->length().audioBytes() + 44;
            }
            else {
                filesize = m_optionWidget->encoder()->fileSize( extension, track->length() );
            }

            if( filesize > 0 )
                overallSize += filesize;

            QString filename = K3b::PatternParser::parsePattern( cddbEntry, track->trackNumber(),
                                                                 extension,
                                                                 m_patternWidget->filenamePattern(),
                                                                 m_patternWidget->replaceBlanks(),
                                                                 m_patternWidget->blankReplaceString() );

            QTreeWidgetItem* item = new QTreeWidgetItem( d->viewTracks );
            item->setText( 0, filename );
            item->setText( 1, track->length().toString() );
            item->setText( 2, filesize < 0 ? i18n("unknown") : KIO::convertSize( filesize ) );

            d->filenames[ track->trackNumber()-1 ] = K3b::fixupPath( baseDir + '/' + filename );
        }
    }

    // create playlist item
    if( m_optionWidget->createPlaylist() ) {
        QString filename = K3b::PatternParser::parsePattern( cddbEntry, 1,
                                                             QLatin1String( "m3u" ),
                                                             m_patternWidget->playlistPattern(),
                                                             m_patternWidget->replaceBlanks(),
                                                             m_patternWidget->blankReplaceString() );

        QTreeWidgetItem* item = new QTreeWidgetItem( d->viewTracks );
        item->setText( 0, filename );
        item->setText( 1, "-" );
        item->setText( 2, "-" );

        d->playlistFilename = K3b::fixupPath( baseDir + '/' + filename );
    }

    if( overallSize > 0 )
        m_optionWidget->setNeededSize( overallSize );
    else
        m_optionWidget->setNeededSize( 0 );
}


void K3b::AudioProjectConvertingDialog::setBaseDir( const QString& path )
{
    m_optionWidget->setBaseDir( path );
}


void K3b::AudioProjectConvertingDialog::loadSettings( const KConfigGroup& c )
{
    m_optionWidget->loadConfig( c );
    m_patternWidget->loadConfig( c );

    refresh();
}


void K3b::AudioProjectConvertingDialog::saveSettings( KConfigGroup c )
{
    m_optionWidget->saveConfig( c );
    m_patternWidget->saveConfig( c );
}


