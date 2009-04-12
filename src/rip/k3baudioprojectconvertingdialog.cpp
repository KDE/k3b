/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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

#include <k3baudiodoc.h>
#include <k3baudioview.h>
#include <k3baudiotrackplayer.h>
#include <k3baudiotrack.h>
#include <k3bjobprogressdialog.h>
#include <k3bcore.h>
#include <k3bglobals.h>
#include <k3blistview.h>
#include <k3baudioencoder.h>

#include <kcombobox.h>
#include <klocale.h>
#include <kconfig.h>
#include <k3listview.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kio/global.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <q3groupbox.h>
#include <q3header.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qtabwidget.h>
//Added by qt3to4:
#include <QGridLayout>

#include <libkcddb/cdinfo.h>


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

    QVector<QString> filenames;
    QString playlistFilename;
    QString cueFilename;
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
    Form1Layout->setMargin( 0 );

    m_viewTracks = new K3b::ListView( frame );
    m_viewTracks->addColumn(i18n( "Filename (relative to base directory)") );
    m_viewTracks->addColumn(i18n( "Length") );
    m_viewTracks->addColumn(i18n( "File Size") );
    m_viewTracks->setSorting(-1);
    m_viewTracks->setAllColumnsShowFocus(true);
    m_viewTracks->setFullWidth(true);

    QTabWidget* mainTab = new QTabWidget( frame );

    m_optionWidget = new K3b::AudioConvertingOptionWidget( mainTab );
    mainTab->addTab( m_optionWidget, i18n("Settings") );


    // setup filename pattern page
    // -------------------------------------------------------------------------------------------
    m_patternWidget = new K3b::CddbPatternWidget( mainTab );
    mainTab->addTab( m_patternWidget, i18n("File Naming") );
    connect( m_patternWidget, SIGNAL(changed()), this, SLOT(refresh()) );

    Form1Layout->addWidget( m_viewTracks, 0, 0 );
    Form1Layout->addWidget( mainTab, 1, 0 );
    Form1Layout->setRowStretch( 0, 1 );

    connect( m_optionWidget, SIGNAL(changed()), this, SLOT(refresh()) );
}


void K3b::AudioProjectConvertingDialog::slotStartClicked()
{
    // make sure we have the tracks just for ourselves
#ifdef __GNUC__
#warning FIXME kde4: port audio player
#endif
    //static_cast<K3b::AudioView*>(m_doc->view())->player()->stop();

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
    Q3ListViewItemIterator it( m_viewTracks );
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
                                                    i18n("Files Exist"), KGuiItem(i18n("Overwrite")) ) == KMessageBox::Cancel )
            return;


    // just generate a fake m_tracks list for now so we can keep most of the methods
    // like they are in K3b::AudioRipJob. This way future combination is easier
    QVector<QPair<int, QString> > tracksToRip;
    int i = 0;
    K3b::AudioTrack* track = m_doc->firstTrack();
    while( track ) {
        tracksToRip.append( qMakePair( i+1, d->filenames[(m_optionWidget->createSingleFile() ? 0 : i)] ) );
        ++i;
        track = track->next();
    }

    K3b::AudioEncoder* encoder = m_optionWidget->encoder();

    K3b::JobProgressDialog progressDialog( parentWidget() );
    K3b::AudioProjectConvertingJob job( m_doc, &progressDialog, 0 );
    job.setCddbEntry( createCddbEntryFromDoc( m_doc ) );
    job.setTracksToRip( tracksToRip );
    job.setSingleFile( m_optionWidget->createSingleFile() );
    job.setWriteCueFile( m_optionWidget->createCueFile() );
    job.setEncoder( encoder );
    job.setWritePlaylist( m_optionWidget->createPlaylist() );
    job.setPlaylistFilename( d->playlistFilename );
    job.setUseRelativePathInPlaylist( m_optionWidget->playlistRelativePath() );
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
    m_viewTracks->clear();
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
                                                     m_patternWidget->filenamePattern(),
                                                     m_patternWidget->replaceBlanks(),
                                                     m_patternWidget->blankReplaceString() );


        (void)new K3ListViewItem( m_viewTracks,
                                  m_viewTracks->lastItem(),
                                  filename,
                                  m_doc->length().toString(),
                                  filesize < 0 ? i18n("unknown") : KIO::convertSize( filesize ) );

        d->filenames.append( K3b::fixupPath( baseDir + "/" + filename ) );

        if( m_optionWidget->createCueFile() ) {
            QString cueFilename = K3b::PatternParser::parsePattern( cddbEntry, 1,
                                                                    QLatin1String( "cue" ),
                                                                    m_patternWidget->filenamePattern(),
                                                                    m_patternWidget->replaceBlanks(),
                                                                    m_patternWidget->blankReplaceString() );

            d->cueFilename = K3b::fixupPath( baseDir + "/" + cueFilename );
            (void)new K3ListViewItem( m_viewTracks,
                                      m_viewTracks->lastItem(),
                                      cueFilename,
                                      "-",
                                      "-",
                                      i18n("Cue-file") );
        }
    }
    else {
        K3b::AudioTrack* track = m_doc->firstTrack();
        unsigned int i = 1;
        while( track ) {
            long long filesize = 0;
            if( m_optionWidget->encoder() == 0 ) {
                filesize = track->length().audioBytes() + 44;
            }
            else {
                filesize = m_optionWidget->encoder()->fileSize( extension, track->length() );
            }

            if( filesize > 0 )
                overallSize += filesize;

            QString filename = K3b::PatternParser::parsePattern( cddbEntry, i,
                                                                 extension,
                                                                 m_patternWidget->filenamePattern(),
                                                                 m_patternWidget->replaceBlanks(),
                                                                 m_patternWidget->blankReplaceString() );

            (void)new K3ListViewItem( m_viewTracks,
                                      m_viewTracks->lastItem(),
                                      filename,
                                      track->length().toString(),
                                      filesize < 0 ? i18n("unknown") : KIO::convertSize( filesize ) );

            d->filenames.append( K3b::fixupPath( baseDir + "/" + filename ) );

            track = track->next();
            ++i;
        }
    }

    // create playlist item
    if( m_optionWidget->createPlaylist() ) {
        QString filename = K3b::PatternParser::parsePattern( cddbEntry, 1,
                                                             QLatin1String( "m3u" ),
                                                             m_patternWidget->playlistPattern(),
                                                             m_patternWidget->replaceBlanks(),
                                                             m_patternWidget->blankReplaceString() );

        (void)new K3ListViewItem( m_viewTracks,
                                  m_viewTracks->lastItem(),
                                  filename,
                                  "-",
                                  "-",
                                  i18n("Playlist") );

        d->playlistFilename = K3b::fixupPath( baseDir + "/" + filename );
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

#include "k3baudioprojectconvertingdialog.moc"
