/*
 *
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiotrackaddingdialog.h"
#include "k3baudiofileanalyzerjob.h"

#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiofile.h"
#include "k3baudiodecoder.h"
#include "k3bbusywidget.h"
#include "k3bglobals.h"
#include "k3bcuefileparser.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>


K3b::AudioTrackAddingDialog::AudioTrackAddingDialog( const QList<QUrl>& urls,
                                                     AudioDoc* doc,
                                                     AudioTrack* afterTrack,
                                                     AudioTrack* parentTrack,
                                                     AudioDataSource* afterSource,
                                                     QWidget* parent )
    : QDialog( parent),
      m_urls( AudioDoc::extractUrlList( urls ) ),
      m_doc( doc ),
      m_trackAfter( afterTrack ),
      m_parentTrack( parentTrack ),
      m_sourceAfter( afterSource ),
      m_bCanceled( false )
{
    setWindowTitle(i18n("Please be patient..."));

    m_infoLabel = new QLabel( this );
    m_infoLabel->setText( i18n("Adding files to project \"%1\"...",doc->URL().fileName()) );
    m_busyWidget = new K3b::BusyWidget( this );
    m_busyWidget->showBusy( true );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Cancel, this );
    connect( buttonBox, SIGNAL(rejected()), SLOT(reject()) );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->addWidget( m_infoLabel );
    layout->addWidget( m_busyWidget );
    layout->addWidget( buttonBox );

    m_analyserJob = new K3b::AudioFileAnalyzerJob( this, this );
    connect( m_analyserJob, SIGNAL(finished(bool)), this, SLOT(slotAnalysingFinished(bool)) );
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(slotCancelClicked()));
}


K3b::AudioTrackAddingDialog::~AudioTrackAddingDialog()
{
    QString message;
    if( !m_unreadableFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br>%2")
                   .arg( i18n("Insufficient permissions to read the following files") )
                   .arg( m_unreadableFiles.join( "<br>" ) );
    if( !m_notFoundFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br>%2")
                   .arg( i18n("Unable to find the following files") )
                   .arg( m_notFoundFiles.join( "<br>" ) );
    if( !m_nonLocalFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br>%2")
                   .arg( i18n("No non-local files supported") )
                   .arg( m_unreadableFiles.join( "<br>" ) );
    if( !m_unsupportedFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br><i>%2</i><br>%3")
                   .arg( i18n("Unable to handle the following files due to an unsupported format" ) )
                   .arg( i18n("You may manually convert these audio files to wave using another "
                              "application supporting the audio format and then add the wave files "
                              "to the K3b project.") )
                   .arg( m_unsupportedFiles.join( "<br>" ) );

    if( !message.isEmpty() )
        KMessageBox::detailedSorry( parentWidget(), i18n("Problems while adding files to the project."), message );
}


void K3b::AudioTrackAddingDialog::addUrls( const QList<QUrl>& urls,
                                          K3b::AudioDoc* doc,
                                          K3b::AudioTrack* afterTrack,
                                          K3b::AudioTrack* parentTrack,
                                          K3b::AudioDataSource* afterSource,
                                          QWidget* parent )
{
    if( !urls.isEmpty() )
    {
        K3b::AudioTrackAddingDialog* dlg = new K3b::AudioTrackAddingDialog(
            urls, doc, afterTrack, parentTrack, afterSource, parent );
        dlg->setAttribute( Qt::WA_DeleteOnClose );
        QMetaObject::invokeMethod( dlg, "exec", Qt::QueuedConnection );
        QMetaObject::invokeMethod( dlg, "slotAddUrls", Qt::QueuedConnection );
    }
}


void K3b::AudioTrackAddingDialog::slotAddUrls()
{
    if( m_bCanceled )
        return;

    if( m_urls.isEmpty() ) {
        accept();
        return;
    }

    QUrl url = m_urls.first();
    bool valid = true;

    if( url.toLocalFile().right(3).toLower() == "cue" ) {
        // see if its a cue file
        K3b::CueFileParser parser( url.toLocalFile() );
        if( parser.isValid() && parser.toc().contentType() == K3b::Device::AUDIO ) {
            if ( parser.imageFileType() == QLatin1String( "bin" ) ) {
                // no need to analyze -> raw audio data
                m_doc->importCueFile( url.toLocalFile(), m_trackAfter, 0 );
                m_urls.erase( m_urls.begin() );
                QMetaObject::invokeMethod( this, "slotAddUrls", Qt::QueuedConnection );
                return;
            }
            else {
                // remember cue url and set the new audio file url
                m_cueUrl = url;
                url = m_urls[0] = QUrl::fromLocalFile( parser.imageFilename() );
            }
        }
    }

    m_infoLabel->setText( i18n("Analysing file '%1'..." , url.fileName() ) );

    if( !url.isLocalFile() ) {
        valid = false;
        m_nonLocalFiles.append( url.toLocalFile() );
    }
    else {
        QFileInfo fi( url.toLocalFile() );
        if( !fi.exists() ) {
            valid = false;
            m_notFoundFiles.append( url.toLocalFile() );
        }
        else if( !fi.isReadable() ) {
            valid = false;
            m_unreadableFiles.append( url.toLocalFile() );
        }
    }

    if( valid ) {
        bool reused;
        K3b::AudioDecoder* dec = m_doc->getDecoderForUrl( url, &reused );
        if( dec ) {
            m_analyserJob->setDecoder( dec );
            if( reused )
                slotAnalysingFinished( true );
            else
                m_analyserJob->start();
        }
        else {
            valid = false;
            m_unsupportedFiles.append( url.toLocalFile() );
        }
    }

    // invalid file, next url
    if( !valid ) {
        m_urls.erase( m_urls.begin() );
        QMetaObject::invokeMethod( this, "slotAddUrls", Qt::QueuedConnection );
    }
}


void K3b::AudioTrackAddingDialog::slotAnalysingFinished( bool /*success*/ )
{
    if( m_bCanceled ) {
        // We only started the analyser thread in case the decoder was new
        // thus, we can safely delete it since no other source needs it.
        delete m_analyserJob->decoder();
        return;
    }

    QUrl url = m_urls.first();
    m_urls.erase( m_urls.begin() );

    if( m_cueUrl.isValid() ) {
        // import the cue file
        m_doc->importCueFile( m_cueUrl.toLocalFile(), m_trackAfter, m_analyserJob->decoder() );
        m_cueUrl = QUrl();
    }
    else {
        // create the track and source items
        K3b::AudioDecoder* dec = m_analyserJob->decoder();
        K3b::AudioFile* file = new K3b::AudioFile( dec, m_doc );
        if( m_parentTrack ) {
            if( m_sourceAfter )
                file->moveAfter( m_sourceAfter );
            else
                file->moveAhead( m_parentTrack->firstSource() );
            m_sourceAfter = file;
        }
        else {
            K3b::AudioTrack* track = new K3b::AudioTrack( m_doc );
            track->setFirstSource( file );

            track->setTitle( dec->metaInfo( K3b::AudioDecoder::META_TITLE ) );
            track->setArtist( dec->metaInfo( K3b::AudioDecoder::META_ARTIST ) );
            track->setSongwriter( dec->metaInfo( K3b::AudioDecoder::META_SONGWRITER ) );
            track->setComposer( dec->metaInfo( K3b::AudioDecoder::META_COMPOSER ) );
            track->setCdTextMessage( dec->metaInfo( K3b::AudioDecoder::META_COMMENT ) );

            if( m_trackAfter )
                track->moveAfter( m_trackAfter );
            else if ( m_doc->lastTrack() )
                track->moveAfter( m_doc->lastTrack() );
            else
                m_doc->addTrack( track, 0 );

            m_trackAfter = track;
        }
    }

    QMetaObject::invokeMethod( this, "slotAddUrls", Qt::QueuedConnection );
}


void K3b::AudioTrackAddingDialog::slotCancelClicked()
{
    m_bCanceled = true;
    m_analyserJob->cancel();
    m_analyserJob->wait();
}


