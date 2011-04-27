/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009-2011 Michal Malek <michalm@jabster.pl>
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


#include "k3baudioripjob.h"
#include "k3bpatternparser.h"

#include "k3bcdparanoialib.h"
#include "k3bjob.h"
#include "k3baudioencoder.h"
#include "k3bwavefilewriter.h"
#include "k3bglobalsettings.h"
#include "k3bcore.h"
#include "k3bcuefilewriter.h"

#include "k3bdevice.h"
#include "k3btoc.h"
#include "k3btrack.h"
#include "k3bglobals.h"

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTimer>

#include <KDebug>
#include <KLocale>
#include <KStandardDirs>


namespace
{

    struct SortByTrackNumber
    {
        SortByTrackNumber( K3b::AudioRipJob::Tracks const& tracks ) : m_tracks( tracks ) {}

        bool operator()( QString const& lhs, QString const& rhs )
        {
            return m_tracks.value( lhs ) < m_tracks.value( rhs );
        }

        K3b::AudioRipJob::Tracks const& m_tracks;
    };

} // namespace


class K3b::AudioRipJob::Private
{
public:
    Private()
        : currentTrack(tracks.end()),
          paranoiaRetries(5),
          neverSkip(false),
          encoder(0),
          waveFileWriter(0),
          paranoiaLib(0),
          device(0),
          useIndex0(false),
          relativePathInPlaylist(false),
          writeCueFile(false) {
    }

    Tracks tracks;
    Tracks::const_iterator currentTrack; // the currently ripped track in m_tracks
    QHash<QString,Msf> lengths;
    long overallSectorsRead;
    long overallSectorsToRead;

    int paranoiaMode;
    int paranoiaRetries;
    int neverSkip;

    K3b::AudioEncoder* encoder;
    K3b::WaveFileWriter* waveFileWriter;

    K3b::CdparanoiaLib* paranoiaLib;

    K3b::Device::Toc toc;

    QString fileType;

    KCDDB::CDInfo cddbEntry;
    Device::Device* device;

    bool useIndex0;

    QString playlistFilename;
    bool relativePathInPlaylist;
    bool writeCueFile;
};


K3b::AudioRipJob::AudioRipJob( K3b::JobHandler* hdl, QObject* parent )
    :  K3b::ThreadJob( hdl, parent )
{
    d = new Private();
}


K3b::AudioRipJob::~AudioRipJob()
{
    delete d->waveFileWriter;
    delete d->paranoiaLib;
    delete d;
}


void K3b::AudioRipJob::setParanoiaMode( int mode )
{
    d->paranoiaMode = mode;
}


void K3b::AudioRipJob::setMaxRetries( int r )
{
    d->paranoiaRetries = r;
}


void K3b::AudioRipJob::setNeverSkip( bool b )
{
    d->neverSkip = b;
}


void K3b::AudioRipJob::setEncoder( K3b::AudioEncoder* f )
{
    d->encoder = f;
}


void K3b::AudioRipJob::setFileType( const QString& t )
{
    d->fileType = t;
}


void K3b::AudioRipJob::setTracksToRip( const Tracks& tracks )
{
    d->tracks = tracks;
}


void K3b::AudioRipJob::setUseIndex0( bool b )
{
    d->useIndex0 = b;
}


void K3b::AudioRipJob::setDevice( Device::Device* device )
{
    d->device = device;
}


void K3b::AudioRipJob::setCddbEntry( const KCDDB::CDInfo& e )
{
    d->cddbEntry = e;
}


void K3b::AudioRipJob::setWritePlaylist( const QString& filename, bool relativePaths )
{
    d->playlistFilename = filename;
    d->relativePathInPlaylist = relativePaths;
}


void K3b::AudioRipJob::setWriteCueFile( bool b )
{
    d->writeCueFile = b;
}


void K3b::AudioRipJob::start()
{
    k3bcore->blockDevice( d->device );
    K3b::ThreadJob::start();
}


void K3b::AudioRipJob::jobFinished( bool success )
{
    if ( !success && canceled() ) {
        if( d->currentTrack != d->tracks.constEnd() ) {
            if( QFile::exists( d->currentTrack.key() ) ) {
                QFile::remove( d->currentTrack.key() );
                emit infoMessage( i18n("Removed partial file '%1'.",d->currentTrack.key()), K3b::Job::MessageInfo );
            }
        }
    }

    k3bcore->unblockDevice( d->device );
    K3b::ThreadJob::jobFinished( success );
}


bool K3b::AudioRipJob::run()
{
    emit newTask( i18n("Extracting Digital Audio")  );

    if( !d->paranoiaLib ) {
        d->paranoiaLib = K3b::CdparanoiaLib::create();
    }

    if( !d->paranoiaLib ) {
        emit infoMessage( i18n("Could not load libcdparanoia."), K3b::Job::MessageError );
        return false;
    }

    // try to open the device
    if( !d->device ) {
        return false;
    }

    d->device->block(true);

    emit infoMessage( i18n("Reading CD table of contents."), K3b::Job::MessageInfo );
    d->toc = d->device->readToc();

    if( !d->paranoiaLib->initParanoia( d->device, d->toc ) ) {
        emit infoMessage( i18n("Could not open device %1",d->device->blockDeviceName()),
                          K3b::Job::MessageError );
        d->device->block(false);

        return false;
    }

    d->paranoiaLib->setParanoiaMode( d->paranoiaMode );
    d->paranoiaLib->setNeverSkip( d->neverSkip );
    d->paranoiaLib->setMaxRetries( d->paranoiaRetries );


    if( !d->encoder && !d->waveFileWriter ) {
        d->waveFileWriter = new K3b::WaveFileWriter();
    }


    if( d->useIndex0 ) {
        emit newSubTask( i18n("Searching index 0 for all tracks") );
        d->device->indexScan( d->toc );
    }

    d->overallSectorsRead = 0;
    d->overallSectorsToRead = 0;
    d->lengths.clear();

    Q_FOREACH( const QString& filename, d->tracks.keys().toSet() ) {
        d->lengths.insert( filename, 0 );
        Q_FOREACH( int track, d->tracks.values( filename ) ) {
            Msf length = d->useIndex0 ? d->toc[track-1].realAudioLength() : d->toc[track-1].length();
            d->lengths[ filename ] += length;
            d->overallSectorsToRead += length.lba();
        }
    }

    emit infoMessage( i18n("Starting digital audio extraction (ripping)."), K3b::Job::MessageInfo );

    bool success = true;
    QString lastFilename;
    for( d->currentTrack = d->tracks.constBegin(); d->currentTrack != d->tracks.constEnd(); ++d->currentTrack ) {
        if( !ripTrack( d->currentTrack.value(), d->currentTrack.key(), lastFilename ) ) {
            success = false;
            break;
        }
        lastFilename = d->currentTrack.key();
    }

    if( d->encoder ) {
        d->encoder->closeFile();
    }
    if( d->waveFileWriter ) {
        d->waveFileWriter->close();
    }

    if( !canceled() && !d->playlistFilename.isNull() ) {
        success = success && writePlaylist();
    }

    if( !canceled() && d->writeCueFile ) {
        if( !d->useIndex0 ) {
            emit newSubTask( i18n("Searching index 0 for all tracks") );
            d->device->indexScan( d->toc );
        }
        success = success && writeCueFile();
    }

    d->paranoiaLib->close();
    d->device->block(false);

    if( canceled() ) {
        return false;
    }
    else {
        return success;
    }
}


bool K3b::AudioRipJob::ripTrack( int track, const QString& filename, const QString& prevFilename )
{
    const K3b::Device::Track& tt = d->toc[track-1];

    long endSec = ( (d->useIndex0 && tt.index0() > 0)
                    ? tt.firstSector().lba() + tt.index0().lba() - 1
                    : tt.lastSector().lba() );

    if( d->paranoiaLib->initReading( tt.firstSector().lba(), endSec ) ) {

        long trackSectorsRead = 0;

        QString dir = filename.left( filename.lastIndexOf('/') );
        if( !KStandardDirs::makeDir( dir, 0777 ) ) {
            emit infoMessage( i18n("Unable to create folder %1", dir), K3b::Job::MessageError );
            return false;
        }

        // Close the previous file if the new filename is different
        if( prevFilename != filename ) {
            if( d->encoder )
                d->encoder->closeFile();
            if( d->waveFileWriter )
                d->waveFileWriter->close();
        }

        // Open the file to write if it is not already opened
        if( (d->encoder && !d->encoder->isOpen()) ||
            (d->waveFileWriter && !d->waveFileWriter->isOpen()) ) {
            bool isOpen = true;
            if( d->encoder ) {
                AudioEncoder::MetaData metaData;
                metaData.insert( AudioEncoder::META_ALBUM_ARTIST, d->cddbEntry.get( KCDDB::Artist ) );
                metaData.insert( AudioEncoder::META_ALBUM_TITLE, d->cddbEntry.get( KCDDB::Title ) );
                metaData.insert( AudioEncoder::META_ALBUM_COMMENT, d->cddbEntry.get( KCDDB::Comment ) );
                metaData.insert( AudioEncoder::META_YEAR, d->cddbEntry.get( KCDDB::Year ) );
                metaData.insert( AudioEncoder::META_GENRE, d->cddbEntry.get( KCDDB::Genre ) );
                if( d->tracks.count( filename ) == 1 ) {
                    metaData.insert( AudioEncoder::META_TRACK_NUMBER, QString::number(track).rightJustified( 2, '0' ) );
                    metaData.insert( AudioEncoder::META_TRACK_ARTIST, d->cddbEntry.track( track-1 ).get( KCDDB::Artist ) );
                    metaData.insert( AudioEncoder::META_TRACK_TITLE, d->cddbEntry.track( track-1 ).get( KCDDB::Title ) );
                    metaData.insert( AudioEncoder::META_TRACK_COMMENT, d->cddbEntry.track( track-1 ).get( KCDDB::Comment ) );
                }
                else {
                    metaData.insert( AudioEncoder::META_TRACK_ARTIST, d->cddbEntry.get( KCDDB::Artist ) );
                    metaData.insert( AudioEncoder::META_TRACK_TITLE, d->cddbEntry.get( KCDDB::Title ) );
                    metaData.insert( AudioEncoder::META_TRACK_COMMENT, d->cddbEntry.get( KCDDB::Comment ) );
                }

                isOpen = d->encoder->openFile( d->fileType, filename, d->lengths[ filename ], metaData );
                if( !isOpen )
                    emit infoMessage( d->encoder->lastErrorString(), K3b::Job::MessageError );
            }
            else {
                isOpen = d->waveFileWriter->open( filename );
            }

            if( !isOpen ) {
                emit infoMessage( i18n("Unable to open '%1' for writing.", filename), K3b::Job::MessageError );
                return false;
            }
        }

        if( !d->cddbEntry.track( track-1 ).get( KCDDB::Artist ).toString().isEmpty() &&
            !d->cddbEntry.track( track-1 ).get( KCDDB::Title ).toString().isEmpty() )
            emit newSubTask( i18n( "Ripping track %1 (%2 - %3)",
                                   track,
                                   d->cddbEntry.track( track-1 ).get( KCDDB::Artist ).toString(),
                                   d->cddbEntry.track( track-1 ).get( KCDDB::Title ).toString() ) );
        else
            emit newSubTask( i18n("Ripping track %1", track) );

        int status;
        while( 1 ) {
            if( canceled() ) {
                return false;
            }

            char* buf = d->paranoiaLib->read( &status );
            if( status == K3b::CdparanoiaLib::S_OK ) {
                if( buf == 0 ) {
                    emit infoMessage( i18n("Successfully ripped track %1 to %2.", track, filename), K3b::Job::MessageInfo );
                    return true;
                }
                else {
                    if( d->encoder ) {
                        if( d->encoder->encode( buf,
                                                CD_FRAMESIZE_RAW ) < 0 ) {
                            kDebug() << "(K3b::AudioRipJob) error while encoding.";
                            emit infoMessage( d->encoder->lastErrorString(), K3b::Job::MessageError );
                            emit infoMessage( i18n("Error while encoding track %1.",track), K3b::Job::MessageError );
                            return false;
                        }
                    }
                    else
                        d->waveFileWriter->write( buf,
                                                  CD_FRAMESIZE_RAW,
                                                  K3b::WaveFileWriter::LittleEndian );

                    trackSectorsRead++;
                    d->overallSectorsRead++;
                    emit subPercent( 100*trackSectorsRead/d->toc[track-1].length().lba() );
                    emit percent( 100*d->overallSectorsRead/d->overallSectorsToRead );
                }
            }
            else {
                emit infoMessage( i18n("Unrecoverable error while ripping track %1.",track), K3b::Job::MessageError );
                return false;
            }
        }
        return true;
    }
    else {
        emit infoMessage( i18n("Error while initializing audio ripping."), K3b::Job::MessageError );
        return false;
    }
}


bool K3b::AudioRipJob::writePlaylist()
{
    // this is an absolut path so there is always a "/"
    QString playlistDir = d->playlistFilename.left( d->playlistFilename.lastIndexOf( '/' ) );

    if( !KStandardDirs::makeDir( playlistDir ) ) {
        emit infoMessage( i18n("Unable to create folder %1",playlistDir), K3b::Job::MessageError );
        return false;
    }

    emit infoMessage( i18n("Writing playlist to %1.", d->playlistFilename ), K3b::Job::MessageInfo );

    QFile f( d->playlistFilename );
    if( f.open( QIODevice::WriteOnly ) ) {
        QTextStream t( &f );

        // format descriptor
        t << "#EXTM3U" << endl;

        // Get list of the ripped filenames sorted by track number
        QStringList filenames = d->tracks.keys();
        filenames.removeDuplicates();
        qSort( filenames.begin(), filenames.end(), SortByTrackNumber( d->tracks ) );

        Q_FOREACH( const QString& filename, filenames ) {

            // extra info
            t << "#EXTINF:" << d->lengths[filename].totalFrames()/75 << ",";

            QVariant artist;
            QVariant title;

            QList<int> trackNums = d->tracks.values( filename );
            if( trackNums.count() == 1 ) {
                int trackIndex = trackNums.first()-1;
                artist = d->cddbEntry.track( trackIndex ).get( KCDDB::Artist );
                title = d->cddbEntry.track( trackIndex ).get( KCDDB::Title );
            }
            else {
                artist = d->cddbEntry.get( KCDDB::Artist );
                title = d->cddbEntry.get( KCDDB::Title );
            }

            if( !artist.toString().isEmpty() && !title.toString().isEmpty() ) {
                t << artist.toString() << " - " << title.toString() << endl;
            }
            else {
                t << filename.mid(filename.lastIndexOf('/') + 1,
                                  filename.length() - filename.lastIndexOf('/') - 5)
                << endl; // filename without extension
            }

            // filename
            if( d->relativePathInPlaylist )
                t << findRelativePath( filename, playlistDir ) << endl;
            else
                t << filename << endl;
        }

        return ( t.status() == QTextStream::Ok );
    }
    else {
        emit infoMessage( i18n("Unable to open '%1' for writing.",d->playlistFilename), K3b::Job::MessageError );
        kDebug() << "(K3b::AudioRipJob) could not open file " << d->playlistFilename << " for writing.";
        return false;
    }
}


bool K3b::AudioRipJob::writeCueFile()
{
    bool success = true;
    Q_FOREACH( const QString& filename, d->tracks.keys().toSet() ) {
        K3b::CueFileWriter cueWriter;

        // create a new toc and cd-text
        K3b::Device::Toc toc;
        K3b::Device::CdText text;
        text.setPerformer( d->cddbEntry.get( KCDDB::Artist ).toString() );
        text.setTitle( d->cddbEntry.get( KCDDB::Title ).toString() );
        K3b::Msf currentSector;

        QList<int> trackNums = d->tracks.values( filename );
        for( int i = 0; i < trackNums.size(); ++i ) {
            int trackNum = trackNums[ i ];
            const K3b::Device::Track& oldTrack = d->toc[trackNum-1];
            K3b::Device::Track newTrack( oldTrack );
            newTrack.setFirstSector( currentSector );
            newTrack.setLastSector( (currentSector+=oldTrack.length()) - 1 );
            toc.append( newTrack );

            text.track(i).setPerformer( d->cddbEntry.track( trackNum-1 ).get( KCDDB::Artist ).toString() );
            text.track(i).setTitle( d->cddbEntry.track( trackNum-1 ).get( KCDDB::Title ).toString() );
        }

        cueWriter.setData( toc );
        cueWriter.setCdText( text );

        // we always use a relative filename here
        QString imageFile = filename.section( '/', -1 );
        cueWriter.setImage( imageFile, ( d->fileType.isEmpty() ? QString("WAVE") : d->fileType ) );

        // use the same base name as the image file
        QString cueFile = filename;
        cueFile.truncate( cueFile.lastIndexOf('.') );
        cueFile += ".cue";

        emit infoMessage( i18n("Writing cue file to %1.",cueFile), K3b::Job::MessageInfo );

        success = success && cueWriter.save( cueFile );
    }
    return success;
}


QString K3b::AudioRipJob::findRelativePath( const QString& absPath, const QString& baseDir )
{
    QString baseDir_ = K3b::prepareDir( K3b::fixupPath(baseDir) );
    QString path = K3b::fixupPath( absPath );

    // both paths have an equal beginning. That's just how it's configured by K3b
    int pos = baseDir_.indexOf( '/' );
    int oldPos = pos;
    while( pos != -1 && path.left( pos+1 ) == baseDir_.left( pos+1 ) ) {
        oldPos = pos;
        pos = baseDir_.indexOf( '/', pos+1 );
    }

    // now the paths are equal up to oldPos, so that's how "deep" we go
    path = path.mid( oldPos+1 );
    baseDir_ = baseDir_.mid( oldPos+1 );
    int numberOfDirs = baseDir_.count( '/' );
    for( int i = 0; i < numberOfDirs; ++i )
        path.prepend( "../" );

    return path;
}


QString K3b::AudioRipJob::jobDescription() const
{
    if( d->cddbEntry.get( KCDDB::Title ).toString().isEmpty() )
        return i18n( "Ripping Audio Tracks" );
    else
        return i18n( "Ripping Audio Tracks From '%1'",
                     d->cddbEntry.get( KCDDB::Title ).toString() );
}


QString K3b::AudioRipJob::jobDetails() const
{
    if( d->encoder )
        return i18np( "1 track (encoding to %2)",
                      "%1 tracks (encoding to %2)",
                      d->tracks.count(),
                      d->encoder->fileTypeComment(d->fileType) );
    else
        return i18np( "1 track", "%1 tracks",
                      d->tracks.count() );
}


QString K3b::AudioRipJob::jobSource() const
{
    if( d->device )
        return d->device->vendor() + " " + d->device->description();
    else
        return QString();
}


QString K3b::AudioRipJob::jobTarget() const
{
    Tracks::const_iterator it = d->tracks.constBegin();
    if( it != d->tracks.constEnd() )
        return QFileInfo( it.key() ).absolutePath();
    else
        return QString();
}

#include "k3baudioripjob.moc"
