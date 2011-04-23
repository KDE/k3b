/*
 * Copyright (C) 2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmetawriter.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdrdaowriter.h"
#include "k3bgrowisofswriter.h"
#include "k3btocfilewriter.h"
#include "k3binffilewriter.h"

#include "k3bexternalbinmanager.h"
#include "k3bglobals.h"
#include "k3btrack.h"
#include "k3btoc.h"
#include "k3bmedium.h"
#include "k3bcore.h"
#include "k3bmediacache.h"
#include "k3bdevicetypes.h"
#include "k3bdeviceglobals.h"

#include <QtCore/QFile>

#include <KLocale>


class K3b::MetaWriter::Private
{
public:
    Private()
        : writingApp(WritingAppAuto),
          writingMode(WritingModeAuto),
          clone(false),
          multiSession(false),
          layerBreak(0),
          hideFirstTrack(false),
          supportedWritingMedia(Device::MEDIA_WRITABLE),
          writingJob(0) {
    }

    // member vars set via setXXX methods
    // ----------------------------------
    WritingApp writingApp;
    WritingMode writingMode;
    QString cueFile;
    bool clone;
    bool multiSession;
    Device::CdText cdText;
    qint64 layerBreak;
    bool hideFirstTrack;
    Device::Toc toc;
    Device::MediaTypes supportedWritingMedia;
    QStringList images;
    // ----------------------------------


    // status vars
    // ----------------------------------
    WritingApp usedWritingApp;
    WritingMode usedWritingMode;

    AbstractWriter* writingJob;

    QVector<QString> infFiles;
    QString tocFile;
    // ----------------------------------


    void prepareTempFileNames( const QString& path = QString() )
    {
        infFiles.clear();

        QString prefix = K3b::findUniqueFilePrefix( "k3b_tmp_", path ) + "_";

        for( int i = 0; i < toc.count(); ++i ) {
            infFiles.append( prefix + QString::number( i+1 ).rightJustified( 2, '0' ) + ".inf" );
        }

        tocFile = prefix + ".toc";
    }

    QString tocFileName()
    {
        if( tocFile.isEmpty() )
            prepareTempFileNames();
        return tocFile;
    }

    QString infFileName( int track )
    {
        if( infFiles.count() < track )
            prepareTempFileNames();
        return infFiles.at( track - 1 );
    }

    void cleanupTempFiles()
    {
        for( int i = 0; i < infFiles.count(); ++i ) {
            if( QFile::exists( infFiles[i] ) )
                QFile::remove( infFiles[i] );
        }

        if( QFile::exists( tocFile ) )
            QFile::remove( tocFile );

        tocFile.truncate(0);
    }
};


K3b::MetaWriter::MetaWriter( Device::Device* dev, JobHandler* hdl, QObject* parent )
    : AbstractWriter( dev, hdl,  parent ),
      d(new Private())
{
}


K3b::MetaWriter::~MetaWriter()
{
    delete d->writingJob;
    delete d;
}


QIODevice* K3b::MetaWriter::ioDevice() const
{
    if( d->writingJob )
        return d->writingJob->ioDevice();
    else
        return 0;
}


void K3b::MetaWriter::start()
{
    jobStarted();

    // step 1: see if we are set up correctly
    if( !ensureSettingsIntegrity() ) {
        jobFinished( false );
        return;
    }

    if( !determineUsedAppAndMode() ) {
        jobFinished( false );
        return;
    }

    delete d->writingJob;
    d->writingJob = 0;

    bool success = true;
    switch( d->usedWritingApp ) {
    case K3b::WritingAppCdrecord:
        success = setupCdrecordJob();
        break;
    case K3b::WritingAppCdrdao:
        success = setupCdrdaoJob();
        break;
    case K3b::WritingAppGrowisofs:
        success = setupGrowisofsob();
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    if( !success ) {
        jobFinished( false );
        return;
    }

    informUser();

    connectJob( d->writingJob, SLOT(slotWritingJobFinished(bool)) );
    connect( d->writingJob, SIGNAL(buffer(int)),
             this, SIGNAL(buffer(int)) );
    connect( d->writingJob, SIGNAL(deviceBuffer(int)),
             this, SIGNAL(deviceBuffer(int)) );
    connect( d->writingJob, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)),
             this, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)) );
    connect( d->writingJob, SIGNAL(nextTrack(int, int)),
             this, SIGNAL(nextTrack(int, int)) );

    d->writingJob->start();
}


bool K3b::MetaWriter::ensureSettingsIntegrity()
{
    if( d->toc.isEmpty() && d->cueFile.isEmpty() ) {
        emit infoMessage( QLatin1String("Internal error: job not setup properly: cue file and toc set! "
                                        "The application needs fixing!"), MessageError );
        return false;
    }
    else if( !d->images.isEmpty() && d->images.count() != d->toc.count() ) {
        emit infoMessage( QLatin1String("Internal error: job not setup properly: image count != track count! "
                                        "The application needs fixing!"), MessageError );
        return false;
    }
    else if( d->toc.contentType() == Device::MIXED ) {
        int dtc = 0;
        for( int i = 0; i < d->toc.count(); ++i ) {
            Device::Track track = d->toc[i];
            if( track.type() == Device::Track::TYPE_DATA ) {
                if( i > 0 && i+1 == d->toc.count() ) {
                    emit infoMessage( QLatin1String("Internal error: job not setup properly: can only handle data tracks at the beginning or end of toc! "
                                                    "The application needs fixing!"), MessageError );
                    return false;
                }
                ++dtc;
            }
        }
        if( dtc > 1 ) {
            emit infoMessage( QLatin1String("Internal error: job not setup properly: cannot handle more than one data track in a session! "
                                            "The application needs fixing!"), MessageError );
            return false;
        }
    }

    return true;
}


bool K3b::MetaWriter::determineUsedAppAndMode()
{
    // =============================================
    // Get the burn medium
    // =============================================

    Device::MediaTypes mt = d->supportedWritingMedia;
    // a little bit of restricting
    if( d->writingMode == K3b::WritingModeRestrictedOverwrite ) // we treat DVD+R(W) as restricted overwrite media
        mt = K3b::Device::MEDIA_DVD_RW_OVWR|K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_PLUS_R;
    else if( d->writingMode == K3b::WritingModeIncrementalSequential )
        mt ^= Device::MEDIA_CD_ALL;
    else if( d->writingMode == K3b::WritingModeRaw )
        mt ^= (Device::MEDIA_DVD_ALL|Device::MEDIA_BD_ALL);

    Device::MediaType mediaType = waitForMedium( burnDevice(), Device::STATE_EMPTY, mt );
    if( mediaType == Device::MEDIA_UNKNOWN )
        return false;

    Medium medium = k3bcore->mediaCache()->medium( burnDevice() );


    // =============================================
    // Some values we need later on
    // =============================================

    bool onTheFly = d->cueFile.isEmpty() && d->images.isEmpty();
    bool cdrecordOnTheFly = false;
    bool cdrecordCdText = false;
    bool cdrecordBluRay = false;
    bool cdrecordWodim = false;
    bool growisofsBluRay = false;
    if( k3bcore->externalBinManager()->binObject("cdrecord") ) {
        cdrecordOnTheFly = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "audio-stdin" );
        cdrecordCdText = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "cdtext" );
        cdrecordBluRay = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "blu-ray" );
        cdrecordWodim = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "wodim" );
    }
    if( k3bcore->externalBinManager()->binObject("growisofs") ) {
        growisofsBluRay = k3bcore->externalBinManager()->binObject("growisofs")->hasFeature( "blu-ray" );
    }


    // =============================================
    // Determine writing app
    // =============================================

    d->usedWritingApp = d->writingApp;
    if( d->writingApp == K3b::WritingAppAuto ) {
        if( mediaType & Device::MEDIA_CD_ALL ) {
            if( d->usedWritingMode == K3b::WritingModeSao ) {
                // there are none-DAO writers that are supported by cdrdao
                if( !burnDevice()->dao() ||
                    ( !cdrecordOnTheFly && onTheFly ) ||
                    ( !d->cdText.isEmpty() && !cdrecordCdText ) ||
                    d->hideFirstTrack ) {
                    d->usedWritingApp = K3b::WritingAppCdrdao;
                }
                // cdrecord seems to have problems writing xa 1 disks in dao mode? At least on my system!
                else if( d->cueFile.isEmpty() &&
                         d->toc.first().mode() != Device::Track::MODE1 ) {
                    d->usedWritingApp = K3b::WritingAppCdrdao;
                }
                else {
                    d->usedWritingApp = K3b::WritingAppCdrecord;
                }
            }
            else
                d->usedWritingApp = K3b::WritingAppCdrecord;
        }
        else {
            if ( d->writingApp == K3b::WritingAppCdrdao ) {
                emit infoMessage( i18n( "Cannot write %1 media using %2. Falling back to default application.",
                                        K3b::Device::mediaTypeString( mediaType, true ), QLatin1String("cdrdao") ), MessageWarning );
                d->writingApp = K3b::WritingAppAuto;
            }

            if( d->toc.count() != 1 || d->toc.first().mode() != Device::Track::MODE1 ) {
                emit infoMessage( i18n("DVD and Blu-ray tracks can only be written in MODE1."), MessageWarning );
            }

            if( mediaType & (K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR) &&
                d->multiSession ) {
                // we can only do this with growisofs
                d->usedWritingApp = WritingAppGrowisofs;
            }
            else if( mediaType & Device::MEDIA_DVD_ALL ) {
                // wodim (at least on fedora) doesn't do DVDs all that well, use growisofs instead
                if ( cdrecordWodim ) {
                    d->usedWritingApp = WritingAppGrowisofs;
                }
                else {
                    d->usedWritingApp = WritingAppCdrecord;
                }
            }
            else if( mediaType & Device::MEDIA_BD_ALL ) {
                if( cdrecordBluRay && ! cdrecordWodim ) {
                    d->usedWritingApp = WritingAppCdrecord;
                }
                else if( growisofsBluRay ) {
                    d->usedWritingApp = WritingAppGrowisofs;
                }
                else {
                    emit infoMessage( i18n("Missing Blu-ray support in cdrecord and growisofs. Please update the system."),  MessageError );
                    return false;
                }
            }
        }
    }
    else
        d->usedWritingApp = d->writingApp;


    // =============================================
    // Determine writing mode
    // =============================================

    if( d->writingMode == K3b::WritingModeAuto ) {
        if( mediaType & (Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR) ) {
            d->usedWritingMode = K3b::WritingModeRestrictedOverwrite;
        }
        else if( mediaType & Device::MEDIA_DVD_PLUS_ALL ) {
            d->usedWritingMode = K3b::WritingModeSao;
        }
        else if( mediaType & (K3b::Device::MEDIA_DVD_RW_SEQ|
                              K3b::Device::MEDIA_DVD_RW) ) {
            d->usedWritingMode = K3b::WritingModeIncrementalSequential;
        }
        else if( mediaType & K3b::Device::MEDIA_DVD_MINUS_ALL ) {
            if( d->multiSession )
                d->usedWritingMode = K3b::WritingModeIncrementalSequential;
            else
                d->usedWritingMode = K3b::WritingModeSao;
        }
        else if( mediaType & Device::MEDIA_CD_ALL ) {
            if( !d->cueFile.isEmpty() ) {
                d->usedWritingMode = WritingModeSao;
            }
            else {
                if( d->toc.contentType() == Device::MIXED ) {
                    // when writing mode in one session, TAO is the only choice
                    // otherwise we need to see what kind of session we write
                    d->usedWritingMode = WritingModeTao;
                }

                else if( d->toc.contentType() == Device::DATA ) {
                    //
                    // Data sessions are simple: always SAO except if there is multisession involved
                    // However, if we add a session, even if d->multiSession is false, use TAO!
                    //
                    if( burnDevice()->dao() &&
                        d->toc.first().mode() == K3b::Device::Track::MODE1 &&
                        !d->multiSession &&
                        medium.diskInfo().empty() )
                        d->usedWritingMode = K3b::WritingModeSao;
                    else
                        d->usedWritingMode = K3b::WritingModeTao;
                }
                else {
                    //
                    // there are a lot of writers out there which produce coasters
                    // in dao mode if the CD contains pregaps of length 0 (or maybe already != 2 secs?)
                    //
                    // Also most writers do not accept cuesheets with tracks smaller than 4 seconds (a violation
                    // of the red book standard) in DAO mode.
                    //
                    bool zeroPregap = false;
                    bool less4Sec = false;
                    for( int i = 0; i < d->toc.count(); ++i ) {
                        Device::Track track = d->toc[i];

                        if( track.type() != Device::Track::TYPE_AUDIO )
                            continue;

                        if( track.index0() == 0 && i+1 < d->toc.count() ) // the last track's postgap is always 0
                            zeroPregap = true;

                        if( track.length() < K3b::Msf( 0, 4, 0 ) )
                            less4Sec = true;
                    }

                    //
                    // DAO is always the first choice
                    // RAW second and TAO last
                    // there are none-DAO writers that are supported by cdrdao
                    //
                    // older cdrecord versions do not support the -shorttrack option in RAW writing mode
                    //
                    if( !burnDevice()->dao() && d->usedWritingApp == K3b::WritingAppCdrecord ) {
                        if( !burnDevice()->supportsRawWriting() &&
                            ( !less4Sec || k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "short-track-raw" ) ) )
                            d->usedWritingMode = K3b::WritingModeRaw;
                        else
                            d->usedWritingMode = K3b::WritingModeTao;
                    }
                    else {
                        if( (zeroPregap || less4Sec) && burnDevice()->supportsRawWriting() ) {
                            d->usedWritingMode = K3b::WritingModeRaw;
                            if( less4Sec )
                                emit infoMessage( i18n("Track lengths below 4 seconds violate the Red Book standard."), MessageWarning );
                        }
                        else
                            d->usedWritingMode = K3b::WritingModeSao;
                    }
                }
            }
        }
        else {
            // FIXME: what to use for BD?
            d->usedWritingMode = K3b::WritingModeSao;
        }
    }
    else {
        d->usedWritingMode = d->writingMode;

        if( !(mediaType & (K3b::Device::MEDIA_DVD_RW|K3b::Device::MEDIA_DVD_RW_OVWR|K3b::Device::MEDIA_DVD_RW_SEQ)) &&
            d->writingMode == K3b::WritingModeRestrictedOverwrite ) {
            emit infoMessage( i18n("Restricted Overwrite is not possible with DVD-R media."), MessageInfo );
            return false;
        }
    }


    // =============================================
    // Some final checks and safety nets
    // =============================================

    // on-the-fly writing with cdrecord >= 2.01a13
    if( d->usedWritingApp == K3b::WritingAppCdrecord &&
        onTheFly &&
        !cdrecordOnTheFly ) {
        emit infoMessage( i18n("On-the-fly writing with cdrecord < 2.01a13 not supported."), MessageError );
        return false;
    }

    if( d->usedWritingApp == K3b::WritingAppCdrecord &&
        !d->cdText.isEmpty() ) {
        if( !cdrecordCdText ) {
            emit infoMessage( i18n("Cdrecord %1 does not support CD-Text writing.",
                                   k3bcore->externalBinManager()->binObject("cdrecord")->version()), MessageError );
            return false;
        }
        else if( d->usedWritingMode == K3b::WritingModeTao ) {
            emit infoMessage( i18n("It is not possible to write CD-Text in TAO mode."), MessageWarning );
            return false;
        }
    }

    if( d->usedWritingMode == K3b::WritingModeIncrementalSequential ) {
        if( burnDevice()->featureCurrent( K3b::Device::FEATURE_INCREMENTAL_STREAMING_WRITABLE ) == 0 ) {
            if( !questionYesNo( i18n("Your writer (%1 %2) does not support Incremental Streaming with %3 "
                                     "media. Multisession will not be possible. Continue anyway?",
                                     burnDevice()->vendor(),
                                     burnDevice()->description(),
                                     K3b::Device::mediaTypeString(mediaType, true) ),
                                i18n("No Incremental Streaming") ) ) {
                return false;
            }
            else {
                d->usedWritingMode = K3b::WritingModeSao;
            }
        }
    }

    if( !(mediaType & (K3b::Device::MEDIA_DVD_RW|K3b::Device::MEDIA_DVD_RW_OVWR|K3b::Device::MEDIA_DVD_RW_SEQ)) &&
        d->usedWritingMode == K3b::WritingModeRestrictedOverwrite ) {
        emit infoMessage( i18n("Restricted Overwrite is not possible with DVD-R media."), MessageInfo );
    }

    if( simulate() ) {
        if( mediaType & K3b::Device::MEDIA_DVD_PLUS_ALL ) {
            if( !questionYesNo( i18n("DVD+R(W) media do not support write simulation. "
                                     "Do you really want to continue? The media will actually be "
                                     "written to."),
                                i18n("No Simulation with DVD+R(W)") ) ) {
                return false;
            }
        }
        else if( mediaType & Device::MEDIA_DVD_MINUS_ALL &&
                 !burnDevice()->dvdMinusTestwrite() ) {
            if( !questionYesNo( i18n("Your writer (%1 %2) does not support simulation with DVD-R(W) media. "
                                     "Do you really want to continue? The media will actually be "
                                     "written to.",
                                     burnDevice()->vendor(),
                                     burnDevice()->description()),
                                i18n("No Simulation with DVD-R(W)") ) ) {
                return false;
            }
        }
    }

    // cdrecord manpage says that "not all" writers are able to write
    // multisession disks in dao mode. That means there are writers that can.

    // Does it really make sence to write Data ms cds in DAO mode since writing the
    // first session of a cd-extra in DAO mode is no problem with my writer while
    // writing the second data session is only possible in TAO mode.
    if( mediaType & Device::MEDIA_CD_ALL &&
        d->usedWritingMode == K3b::WritingModeSao &&
        d->multiSession )
        emit infoMessage( i18n("Most writers do not support writing "
                               "multisession CDs in DAO mode."), MessageWarning );

    kDebug() << "Writing mode:     " << d->writingMode;
    kDebug() << "Used Writing mode:" << d->usedWritingMode;
    kDebug() << "Writing app:     " << d->writingApp;
    kDebug() << "Used Writing app:" << d->usedWritingApp;

    return true;
}


bool K3b::MetaWriter::setupCdrecordJob()
{
    K3b::CdrecordWriter* writer = new K3b::CdrecordWriter( burnDevice(), this, this );
    d->writingJob = writer;

    writer->setWritingMode( d->usedWritingMode );
    writer->setSimulate( simulate() );
    writer->setBurnSpeed( burnSpeed() );
    writer->setMulti( d->multiSession );

    if( d->multiSession &&
        !d->toc.isEmpty() &&
        d->images.isEmpty() ) {
        writer->addArgument("-waiti");
    }

    if( d->cueFile.isEmpty() ) {
        bool firstAudioTrack = true;
        int audioTrackCnt = 0;

        for( int i = 0; i < d->toc.count(); ++i ) {
            Device::Track track = d->toc[i];
            QString image;
            if( d->images.count() )
                image = d->images[i];

            //
            // Add a data track
            //
            if( track.type() == Device::Track::TYPE_DATA ) {
                if( track.mode() == Device::Track::MODE1 ) {
                    writer->addArgument( "-data" );
                }
                else {
                    if( k3bcore->externalBinManager()->binObject("cdrecord") &&
                        k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "xamix" ) )
                        writer->addArgument( "-xa" );
                    else
                        writer->addArgument( "-xa1" );
                }

                if( image.isEmpty() )
                    writer->addArgument( QString("-tsize=%1s").arg(track.length().lba()) )->addArgument("-");
                else
                    writer->addArgument( image );
            }

            //
            // Add an audio track
            //
            else {
                if( firstAudioTrack ) {
                    firstAudioTrack = false;
                    writer->addArgument( "-useinfo" );

                    // add raw cdtext data
                    if( !d->cdText.isEmpty() ) {
                        writer->setRawCdText( d->cdText.rawPackData() );
                    }

                    writer->addArgument( "-audio" );

                    // we always pad because although K3b makes sure all tracks' length are multiples of 2352
                    // it seems that normalize sometimes corrupts these lengths
                    // FIXME: see K3b::AudioJob for the whole less4secs and zeroPregap handling
                    writer->addArgument( "-pad" );

                    // Allow tracks shorter than 4 seconds
                    writer->addArgument( "-shorttrack" );
                }

                K3b::InfFileWriter infFileWriter;
                infFileWriter.setTrack( track );
                infFileWriter.setTrackNumber( ++audioTrackCnt );
                if( image.isEmpty() )
                    infFileWriter.setBigEndian( false );
                if( !infFileWriter.save( d->infFileName( audioTrackCnt ) ) )
                    return false;

                if( image.isEmpty() ) {
                    // this is only supported by cdrecord versions >= 2.01a13
                    writer->addArgument( QFile::encodeName( d->infFileName( audioTrackCnt ) ) );
                }
                else {
                    writer->addArgument( QFile::encodeName( image ) );
                }
            }
        }
    }
    else {
        writer->setCueFile( d->cueFile );
    }

    return true;
}


bool K3b::MetaWriter::setupCdrdaoJob()
{
    QString tocFile = d->cueFile;

    if( !d->cueFile.isEmpty() ) {
        K3b::TocFileWriter tocFileWriter;

        //
        // TOC
        //
        tocFileWriter.setData( d->toc );
        tocFileWriter.setHideFirstTrack( d->hideFirstTrack );

        //
        // CD-Text
        //
        if( !d->cdText.isEmpty() ) {
            Device::CdText text = d->cdText;
            // if data in first track we need to add a dummy cdtext
            if( d->toc.first().type() == Device::Track::TYPE_DATA )
                text.insert( 0, K3b::Device::TrackCdText() );

            tocFileWriter.setCdText( text );
        }

        //
        // image filenames
        //
        tocFileWriter.setFilenames( d->images );

        if( !tocFileWriter.save( d->tocFile ))
            return false;

        tocFile = d->tocFile;
    }


    K3b::CdrdaoWriter* writer = new K3b::CdrdaoWriter( burnDevice(), this, this );
    writer->setSimulate( simulate() );
    writer->setBurnSpeed( burnSpeed() );

    // multisession only for the first session
    writer->setMulti( d->multiSession );

    writer->setTocFile( tocFile );

    d->writingJob = writer;

    return true;
}


bool K3b::MetaWriter::setupGrowisofsob()
{
    K3b::GrowisofsWriter* job = new K3b::GrowisofsWriter( burnDevice(), this, this );

    // these do only make sense with DVD-R(W)
    job->setSimulate( simulate() );
    job->setBurnSpeed( burnSpeed() );
    job->setWritingMode( d->usedWritingMode );
    job->setCloseDvd( !d->multiSession );

    //
    // In case the first layer size is not known let the
    // split be determined by growisofs
    //
    if( d->layerBreak > 0 ) {
        job->setLayerBreak( d->layerBreak );
    }
    else {
        // this is only used in DAO mode with growisofs >= 5.15
        job->setTrackSize( d->toc.first().length().lba() );
    }

    if( d->images.isEmpty() )
        job->setImageToWrite( QString() ); // read from stdin
    else
        job->setImageToWrite( d->images.first() );

    d->writingJob = job;

    return true;
}


void K3b::MetaWriter::cancel()
{
    if( active() ) {
        if( d->writingJob && d->writingJob->active() ) {
            d->writingJob->cancel();
        }
        else {
            // can this really happen?
            emit canceled();
            jobFinished(false);
        }
    }
}


void K3b::MetaWriter::slotWritingJobFinished( bool success )
{
    d->cleanupTempFiles();
    jobFinished( success );
}


void K3b::MetaWriter::setSessionToWrite( const Device::Toc& toc, const QStringList& images )
{
    d->toc = toc;
    d->images = images;
}


void K3b::MetaWriter::setSupportedWritingMedia( Device::MediaTypes types )
{
    d->supportedWritingMedia = types;
}


void K3b::MetaWriter::setWritingApp( WritingApp app )
{
    d->writingApp = app;
}


void K3b::MetaWriter::setWritingMode( WritingMode mode )
{
    d->writingMode = mode;
}


void K3b::MetaWriter::setCueFile( const QString& s)
{
    d->cueFile = s;
}


void K3b::MetaWriter::setClone( bool b )
{
    d->clone = b;
}


void K3b::MetaWriter::setMultiSession( bool b )
{
    d->multiSession = b;
}


void K3b::MetaWriter::setCdText( const Device::CdText& cdtext )
{
    d->cdText = cdtext;
}


void K3b::MetaWriter::setLayerBreak( qint64 lb )
{
    d->layerBreak = lb;
}


void K3b::MetaWriter::setHideFirstTrack( bool b )
{
    d->hideFirstTrack = b;
}


void K3b::MetaWriter::informUser()
{
    Medium medium = k3bcore->mediaCache()->medium( burnDevice() );

    if( medium.diskInfo().mediaType() == Device::MEDIA_CD_R ) {
        if( medium.diskInfo().empty() ) {
            if( d->usedWritingMode == WritingModeSao )
                emit infoMessage( i18n("Writing CD in Session At Once mode."), MessageInfo );
            else if( d->usedWritingMode == WritingModeTao )
                emit infoMessage( i18n("Writing CD in Track At Once mode."), MessageInfo );
            else if( d->usedWritingMode == WritingModeRaw )
                emit infoMessage( i18n("Writing CD in Raw mode."), MessageInfo );
        }
        else {
            emit infoMessage( i18n("Appending session to CD"), MessageInfo );
        }
    }

    else if( medium.diskInfo().mediaType() == Device::MEDIA_CD_RW ) {
        if( medium.diskInfo().empty() ) {
            if( d->usedWritingMode == WritingModeSao )
                emit infoMessage( i18n("Writing rewritable CD in Session At Once mode."), MessageInfo );
            else if( d->usedWritingMode == WritingModeTao )
                emit infoMessage( i18n("Writing rewritable CD in Track At Once mode."), MessageInfo );
            else if( d->usedWritingMode == WritingModeRaw )
                emit infoMessage( i18n("Writing rewritable CD in Raw mode."), MessageInfo );
        }
        else {
            emit infoMessage( i18n("Appending session to rewritable CD."), MessageInfo );
        }
    }

    else if( Device::isDvdMedia( medium.diskInfo().mediaType() ) ) {
        if( medium.diskInfo().appendable() ) {
            if( medium.diskInfo().mediaType() & (Device::MEDIA_DVD_PLUS_RW|Device::MEDIA_DVD_PLUS_RW_DL) )
                emit infoMessage( i18n("Growing ISO9660 filesystem on DVD+RW."), MessageInfo );
            else if( medium.diskInfo().mediaType() == Device::MEDIA_DVD_RW_OVWR )
                emit infoMessage( i18n("Growing ISO9660 filesystem on DVD-RW in restricted overwrite mode."), MessageInfo );
            else if( medium.diskInfo().mediaType() == Device::MEDIA_DVD_PLUS_R )
                emit infoMessage( i18n("Appending session to DVD+R."), MessageInfo );
            else if( medium.diskInfo().mediaType() == Device::MEDIA_DVD_PLUS_R_DL )
                emit infoMessage( i18n("Appending session to Double Layer DVD+R."), MessageInfo );
            else
                emit infoMessage( i18n("Appending session to %1.", K3b::Device::mediaTypeString(medium.diskInfo().mediaType(), true) ), MessageInfo );
        }
        else {
            if( medium.diskInfo().mediaType() == Device::MEDIA_DVD_RW_OVWR )
                emit infoMessage( i18n("Writing DVD-RW in restricted overwrite mode."), MessageInfo );
            else if( medium.diskInfo().mediaType() == Device::MEDIA_DVD_PLUS_R_DL )
                emit infoMessage( i18n("Writing Double Layer DVD+R."), MessageInfo );
            else if( medium.diskInfo().mediaType() & Device::MEDIA_DVD_PLUS_ALL )
                emit infoMessage( i18n("Writing %1.", K3b::Device::mediaTypeString(medium.diskInfo().mediaType(), true)), MessageInfo );
            else if( d->usedWritingMode == WritingModeSao )
                emit infoMessage( i18n("Writing %1 in DAO mode.", K3b::Device::mediaTypeString(medium.diskInfo().mediaType(), true) ), MessageInfo );
            else if( d->usedWritingMode == WritingModeIncrementalSequential )
                emit infoMessage( i18n("Writing %1 in incremental mode.", K3b::Device::mediaTypeString(medium.diskInfo().mediaType(), true) ), MessageInfo );
        }
    }

    else {
        emit infoMessage( i18n("Writing %1.", K3b::Device::mediaTypeString(medium.diskInfo().mediaType(), true) ), MessageInfo );
    }
}


K3b::WritingApp K3b::MetaWriter::usedWritingApp() const
{
    return d->usedWritingApp;
}


K3b::WritingMode K3b::MetaWriter::usedWritingMode() const
{
    return d->usedWritingMode;
}

#include "k3bmetawriter.moc"
