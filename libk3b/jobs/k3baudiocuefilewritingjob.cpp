/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiocuefilewritingjob.h"
#include "k3baudiofileanalyzerjob.h"

#include <k3baudiodoc.h>
#include <k3baudiojob.h>
#include <k3bdevice.h>
#include <k3baudiodecoder.h>
#include <k3baudiotrack.h>
#include <k3baudiofile.h>
#include <k3bcuefileparser.h>
#include <k3bthread.h>
#include <k3bthreadjob.h>

#include <kdebug.h>
#include <klocale.h>


class K3bAudioCueFileWritingJob::Private
{
public:
    Private()
        : device( 0 ),
          audioDoc( 0 ),
          audioJob( 0 ),
          decoder( 0 ),
          analyserJob( 0 ) {
    }

    K3bDevice::Device* device;

    QString cueFile;
    K3bAudioDoc* audioDoc;
    K3bAudioJob* audioJob;
    K3bAudioDecoder* decoder;

    K3bAudioFileAnalyzerJob* analyserJob;

    bool audioJobRunning;
    bool canceled;
};



K3bAudioCueFileWritingJob::K3bAudioCueFileWritingJob( K3bJobHandler* jh, QObject* parent )
    : K3bBurnJob( jh, parent ),
      d( new Private() )
{
    d->analyserJob = new K3bAudioFileAnalyzerJob( this, this );
    connect( d->analyserJob, SIGNAL(finished(bool)),
             this, SLOT(slotAnalyserJobFinished(bool)) );

    d->audioDoc = new K3bAudioDoc( this );
    d->audioDoc->newDocument();
    d->audioJob = new K3bAudioJob( d->audioDoc, this, this );

    // just loop all through
    connect( d->audioJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( d->audioJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( d->audioJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
    connect( d->audioJob, SIGNAL(infoMessage(const QString&, int)),
             this, SIGNAL(infoMessage(const QString&, int)) );
    connect( d->audioJob, SIGNAL(finished(bool)), this, SIGNAL(finished(bool)) );
    connect( d->audioJob, SIGNAL(canceled()), this, SIGNAL(canceled()) );
    connect( d->audioJob, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
    connect( d->audioJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
    connect( d->audioJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( d->audioJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( d->audioJob, SIGNAL(burning(bool)), this, SIGNAL(burning(bool)) );
    connect( d->audioJob, SIGNAL(bufferStatus(int)), this, SIGNAL(bufferStatus(int)) );
    connect( d->audioJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( d->audioJob, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );

    d->canceled = false;
    d->audioJobRunning = false;
}


K3bAudioCueFileWritingJob::~K3bAudioCueFileWritingJob()
{
    delete d;
}


K3bDevice::Device* K3bAudioCueFileWritingJob::writer() const
{
    return d->audioDoc->burner();
}


QString K3bAudioCueFileWritingJob::cueFile() const
{
    return d->cueFile;
}


QString K3bAudioCueFileWritingJob::jobDescription() const
{
    return i18n("Writing Audio Cue File");
}


QString K3bAudioCueFileWritingJob::jobDetails() const
{
    return d->cueFile.section( '/', -1 );
}


void K3bAudioCueFileWritingJob::start()
{
    // FIXME: here we trust that a job won't be started twice :(
    jobStarted();
    d->canceled = false;
    d->audioJobRunning = false;
    importCueInProject();
}


void K3bAudioCueFileWritingJob::cancel()
{
    d->canceled = true;

    // the AudioJob cancel method is very stupid. It emits the canceled signal even if it was never running :(
    if( d->audioJobRunning )
        d->audioJob->cancel();
    d->analyserJob->cancel();
}


void K3bAudioCueFileWritingJob::setCueFile( const QString& s )
{
    d->cueFile = s;
}


void K3bAudioCueFileWritingJob::setOnTheFly( bool b )
{
    d->audioDoc->setOnTheFly( b );
}


void K3bAudioCueFileWritingJob::setSpeed( int s )
{
    d->audioDoc->setSpeed( s );
}


void K3bAudioCueFileWritingJob::setBurnDevice( K3bDevice::Device* dev )
{
    d->audioDoc->setBurner( dev );
}


void K3bAudioCueFileWritingJob::setWritingMode( K3b::WritingMode mode )
{
    d->audioDoc->setWritingMode( mode );
}


void K3bAudioCueFileWritingJob::setSimulate( bool b )
{
    d->audioDoc->setDummy( b );
}


void K3bAudioCueFileWritingJob::setCopies( int c )
{
    d->audioDoc->setCopies( c );
}


void K3bAudioCueFileWritingJob::setTempDir( const QString& s )
{
    d->audioDoc->setTempDir( s );
}


void K3bAudioCueFileWritingJob::slotAnalyserJobFinished( bool )
{
    if( !d->canceled ) {
        if( d->audioDoc->lastTrack()->length() == 0 ) {
            emit infoMessage( i18n("Analysing the audio file failed. Corrupt file?"), ERROR );
            jobFinished(false);
        }
        else {
            // FIXME: d->audioJobRunning is never reset
            d->audioJobRunning = true;
            d->audioJob->start(); // from here on the audio job takes over completely
        }
    }
    else {
        emit canceled();
        jobFinished(false);
    }
}


void K3bAudioCueFileWritingJob::importCueInProject()
{
    // cleanup the project (this wil also delete the decoder)
    // we do not use newDocument as that would overwrite the settings already made
    while( d->audioDoc->firstTrack() )
        delete d->audioDoc->firstTrack()->take();

    d->decoder = 0;

    K3bCueFileParser parser( d->cueFile );
    if( parser.isValid() && parser.toc().contentType() == K3bDevice::AUDIO ) {

        kDebug() << "(K3bAudioCueFileWritingJob::importCueFile) parsed with image: " << parser.imageFilename();

        // global cd-text
        d->audioDoc->setTitle( parser.cdText().title() );
        d->audioDoc->setPerformer( parser.cdText().performer() );
        d->audioDoc->writeCdText( !parser.cdText().title().isEmpty() );

        d->decoder = K3bAudioDecoderFactory::createDecoder( parser.imageFilename() );
        if( d->decoder ) {
            d->decoder->setFilename( parser.imageFilename() );

            K3bAudioTrack* after = 0;
            K3bAudioFile* newFile = 0;
            unsigned int i = 0;
            for( K3bDevice::Toc::const_iterator it = parser.toc().begin();
                 it != parser.toc().end(); ++it ) {
                const K3bDevice::Track& track = *it;

                newFile = new K3bAudioFile( d->decoder, d->audioDoc );
                newFile->setStartOffset( track.firstSector() );
                newFile->setEndOffset( track.lastSector()+1 );

                K3bAudioTrack* newTrack = new K3bAudioTrack( d->audioDoc );
                newTrack->addSource( newFile );
                newTrack->moveAfter( after );

                // cd-text
                newTrack->setTitle( parser.cdText()[i].title() );
                newTrack->setPerformer( parser.cdText()[i].performer() );

                // add the next track after this one
                after = newTrack;
                ++i;
            }

            // let the last source use the data up to the end of the file
            if( newFile )
                newFile->setEndOffset(0);

            // now analyze the source
            emit newTask( i18n("Analysing the audio file") );
            emit newSubTask( i18n("Analysing %1", parser.imageFilename() ) );

            // start the analyser job
            d->analyserJob->setDecoder( d->decoder );
            d->analyserJob->start();
        }
        else {
            emit infoMessage( i18n("Unable to handle '%1' due to an unsupported format.", d->cueFile ), ERROR );
            jobFinished(false);
        }
    }
    else {
        emit infoMessage( i18n("No valid audio cue file: '%1'", d->cueFile ), ERROR );
        jobFinished(false);
    }
}

#include "k3baudiocuefilewritingjob.moc"
