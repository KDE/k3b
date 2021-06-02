/*

    SPDX-FileCopyrightText: 2005-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudiocuefilewritingjob.h"
#include "k3baudiofileanalyzerjob.h"

#include "k3baudiodoc.h"
#include "k3baudiojob.h"
#include "k3bdevice.h"
#include "k3baudiodecoder.h"
#include "k3baudiotrack.h"
#include "k3baudiofile.h"
#include "k3bcuefileparser.h"
#include "k3bthread.h"
#include "k3bthreadjob.h"
#include "k3b_i18n.h"

#include <QDebug>


class K3b::AudioCueFileWritingJob::Private
{
public:
    Private()
        : device( 0 ),
          audioDoc( 0 ),
          audioJob( 0 ),
          decoder( 0 ),
          analyserJob( 0 ) {
    }

    K3b::Device::Device* device;

    QString cueFile;
    K3b::AudioDoc* audioDoc;
    K3b::AudioJob* audioJob;
    K3b::AudioDecoder* decoder;

    K3b::AudioFileAnalyzerJob* analyserJob;

    bool audioJobRunning;
    bool canceled;
};



K3b::AudioCueFileWritingJob::AudioCueFileWritingJob( K3b::JobHandler* jh, QObject* parent )
    : K3b::BurnJob( jh, parent ),
      d( new Private() )
{
    d->analyserJob = new K3b::AudioFileAnalyzerJob( this, this );
    connect( d->analyserJob, SIGNAL(finished(bool)),
             this, SLOT(slotAnalyserJobFinished(bool)) );

    d->audioDoc = new K3b::AudioDoc( this );
    d->audioDoc->newDocument();
    d->audioJob = new K3b::AudioJob( d->audioDoc, this, this );

    // just loop all through
    connect( d->audioJob, SIGNAL(newTask(QString)), this, SIGNAL(newTask(QString)) );
    connect( d->audioJob, SIGNAL(newSubTask(QString)), this, SIGNAL(newSubTask(QString)) );
    connect( d->audioJob, SIGNAL(debuggingOutput(QString,QString)),
             this, SIGNAL(debuggingOutput(QString,QString)) );
    connect( d->audioJob, SIGNAL(infoMessage(QString,int)),
             this, SIGNAL(infoMessage(QString,int)) );
    connect( d->audioJob, SIGNAL(finished(bool)), this, SIGNAL(finished(bool)) );
    connect( d->audioJob, SIGNAL(canceled()), this, SIGNAL(canceled()) );
    connect( d->audioJob, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
    connect( d->audioJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
    connect( d->audioJob, SIGNAL(processedSize(int,int)), this, SIGNAL(processedSubSize(int,int)) );
    connect( d->audioJob, SIGNAL(processedSubSize(int,int)), this, SIGNAL(processedSubSize(int,int)) );
    connect( d->audioJob, SIGNAL(burning(bool)), this, SIGNAL(burning(bool)) );
    connect( d->audioJob, SIGNAL(bufferStatus(int)), this, SIGNAL(bufferStatus(int)) );
    connect( d->audioJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( d->audioJob, SIGNAL(writeSpeed(int,K3b::Device::SpeedMultiplicator)), this, SIGNAL(writeSpeed(int,K3b::Device::SpeedMultiplicator)) );

    d->canceled = false;
    d->audioJobRunning = false;
}


K3b::AudioCueFileWritingJob::~AudioCueFileWritingJob()
{
    delete d;
}


K3b::Device::Device* K3b::AudioCueFileWritingJob::writer() const
{
    return d->audioDoc->burner();
}


QString K3b::AudioCueFileWritingJob::cueFile() const
{
    return d->cueFile;
}


QString K3b::AudioCueFileWritingJob::jobDescription() const
{
    return i18n("Writing Audio Cue File");
}


QString K3b::AudioCueFileWritingJob::jobDetails() const
{
    return d->cueFile.section( '/', -1 );
}


QString K3b::AudioCueFileWritingJob::jobSource() const
{
    return d->cueFile;
}


QString K3b::AudioCueFileWritingJob::jobTarget() const
{
    if( Device::Device* device = writer() )
        return device->vendor() + ' ' + device->description();
    else
        return QString();
}


void K3b::AudioCueFileWritingJob::start()
{
    // FIXME: here we trust that a job won't be started twice :(
    jobStarted();
    d->canceled = false;
    d->audioJobRunning = false;
    importCueInProject();
}


void K3b::AudioCueFileWritingJob::cancel()
{
    d->canceled = true;

    // the AudioJob cancel method is very stupid. It emits the canceled signal even if it was never running :(
    if( d->audioJobRunning )
        d->audioJob->cancel();
    d->analyserJob->cancel();
}


void K3b::AudioCueFileWritingJob::setCueFile( const QString& s )
{
    d->cueFile = s;
}


void K3b::AudioCueFileWritingJob::setOnTheFly( bool b )
{
    d->audioDoc->setOnTheFly( b );
}


void K3b::AudioCueFileWritingJob::setSpeed( int s )
{
    d->audioDoc->setSpeed( s );
}


void K3b::AudioCueFileWritingJob::setBurnDevice( K3b::Device::Device* dev )
{
    d->audioDoc->setBurner( dev );
}


void K3b::AudioCueFileWritingJob::setWritingMode( K3b::WritingMode mode )
{
    d->audioDoc->setWritingMode( mode );
}


void K3b::AudioCueFileWritingJob::setSimulate( bool b )
{
    d->audioDoc->setDummy( b );
}


void K3b::AudioCueFileWritingJob::setCopies( int c )
{
    d->audioDoc->setCopies( c );
}


void K3b::AudioCueFileWritingJob::setTempDir( const QString& s )
{
    d->audioDoc->setTempDir( s );
}


void K3b::AudioCueFileWritingJob::slotAnalyserJobFinished( bool )
{
    if( !d->canceled ) {
        if( d->audioDoc->lastTrack()->length() == 0 ) {
            emit infoMessage( i18n("Analysing the audio file failed. Corrupt file?"), MessageError );
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


void K3b::AudioCueFileWritingJob::importCueInProject()
{
    // cleanup the project (this wil also delete the decoder)
    // we do not use newDocument as that would overwrite the settings already made
    while( d->audioDoc->firstTrack() )
        delete d->audioDoc->firstTrack()->take();

    d->decoder = 0;

    K3b::CueFileParser parser( d->cueFile );
    if( parser.isValid() && parser.toc().contentType() == K3b::Device::AUDIO ) {

        qDebug() << "(K3b::AudioCueFileWritingJob::importCueFile) parsed with image: " << parser.imageFilename();

        // global cd-text
        d->audioDoc->setTitle( parser.cdText().title() );
        d->audioDoc->setPerformer( parser.cdText().performer() );
        d->audioDoc->writeCdText( !parser.cdText().title().isEmpty() );

        d->decoder = K3b::AudioDecoderFactory::createDecoder( QUrl::fromLocalFile(parser.imageFilename()) );
        if( d->decoder ) {
            d->decoder->setFilename( parser.imageFilename() );

            K3b::AudioTrack* after = 0;
            K3b::AudioFile* newFile = 0;
            unsigned int i = 0;
            for( K3b::Device::Toc::const_iterator it = parser.toc().constBegin();
                 it != parser.toc().constEnd(); ++it ) {
                const K3b::Device::Track& track = *it;

                newFile = new K3b::AudioFile( d->decoder, d->audioDoc );
                newFile->setStartOffset( track.firstSector() );
                newFile->setEndOffset( track.lastSector()+1 );

                K3b::AudioTrack* newTrack = new K3b::AudioTrack( d->audioDoc );
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
            emit infoMessage( i18n("Unable to handle '%1' due to an unsupported format.", d->cueFile ), MessageError );
            jobFinished(false);
        }
    }
    else {
        emit infoMessage( i18n("No valid audio cue file: '%1'", d->cueFile ), MessageError );
        jobFinished(false);
    }
}


