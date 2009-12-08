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

#include <qfile.h>
#include <qfileinfo.h>
#include <qtimer.h>

#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>




class K3b::AudioRipJob::Private
{
public:
    Private()
        : paranoiaRetries(5),
          neverSkip(false),
          encoder(0),
          waveFileWriter(0),
          paranoiaLib(0) {
    }

    // the index of the currently ripped track in m_tracks
    int currentTrackIndex;
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
};


K3b::AudioRipJob::AudioRipJob( K3b::JobHandler* hdl, QObject* parent )
    :  K3b::ThreadJob( hdl, parent ),
       m_device(0),
       m_useIndex0(false)
{
    d = new Private();
}


K3b::AudioRipJob::~AudioRipJob()
{
    delete d->waveFileWriter;
    delete d->paranoiaLib;
    delete d;
}


void K3b::AudioRipJob::setFileType( const QString& t )
{
    d->fileType = t;
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


void K3b::AudioRipJob::start()
{
    k3bcore->blockDevice( m_device );
    K3b::ThreadJob::start();
}


void K3b::AudioRipJob::jobFinished( bool success )
{
    if ( !success && canceled() ) {
        if( d->currentTrackIndex >= 0 && d->currentTrackIndex < (int)m_tracks.count() ) {
            if( QFile::exists( m_tracks[d->currentTrackIndex].second ) ) {
                QFile::remove( m_tracks[d->currentTrackIndex].second );
                emit infoMessage( i18n("Removed partial file '%1'.",m_tracks[d->currentTrackIndex].second), K3b::Job::MessageInfo );
            }
        }
    }

    k3bcore->unblockDevice( m_device );
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
    if( !m_device ) {
        return false;
    }

    m_device->block(true);

    emit infoMessage( i18n("Reading CD table of contents."), K3b::Job::MessageInfo );
    d->toc = m_device->readToc();

    if( !d->paranoiaLib->initParanoia( m_device, d->toc ) ) {
        emit infoMessage( i18n("Could not open device %1",m_device->blockDeviceName()),
                          K3b::Job::MessageError );
        m_device->block(false);

        return false;
    }

    d->paranoiaLib->setParanoiaMode( d->paranoiaMode );
    d->paranoiaLib->setNeverSkip( d->neverSkip );
    d->paranoiaLib->setMaxRetries( d->paranoiaRetries );


    if( !d->encoder )
        if( !d->waveFileWriter ) {
            d->waveFileWriter = new K3b::WaveFileWriter();
        }


    if( m_useIndex0 ) {
        emit newSubTask( i18n("Searching index 0 for all tracks") );
        m_device->indexScan( d->toc );
    }


    d->overallSectorsRead = 0;
    d->overallSectorsToRead = 0;
    for( int i = 0; i < m_tracks.count(); ++i ) {
        if( m_useIndex0 )
            d->overallSectorsToRead += d->toc[m_tracks[i].first-1].realAudioLength().lba();
        else
            d->overallSectorsToRead += d->toc[m_tracks[i].first-1].length().lba();
    }


    if( m_singleFile ) {
        QString& filename = m_tracks[0].second;

        QString dir = filename.left( filename.lastIndexOf('/') );
        if( !KStandardDirs::makeDir( dir, 0777 ) ) {
            d->paranoiaLib->close();
            emit infoMessage( i18n("Unable to create folder %1",dir), K3b::Job::MessageError );
            m_device->block(false);
            return false;
        }

        // initialize
        bool isOpen = true;
        if( d->encoder ) {
            isOpen = d->encoder->openFile( d->fileType, filename, d->overallSectorsToRead );
            if( isOpen ) {
                // here we use cd Title and Artist
                d->encoder->setMetaData( K3b::AudioEncoder::META_TRACK_ARTIST, m_cddbEntry.get( KCDDB::Artist ).toString() );
                d->encoder->setMetaData( K3b::AudioEncoder::META_TRACK_TITLE, m_cddbEntry.get( KCDDB::Title ).toString() );
                d->encoder->setMetaData( K3b::AudioEncoder::META_TRACK_COMMENT, m_cddbEntry.get( KCDDB::Comment ).toString() );
                d->encoder->setMetaData( K3b::AudioEncoder::META_ALBUM_ARTIST, m_cddbEntry.get( KCDDB::Artist ).toString() );
                d->encoder->setMetaData( K3b::AudioEncoder::META_ALBUM_TITLE, m_cddbEntry.get( KCDDB::Title ).toString() );
                d->encoder->setMetaData( K3b::AudioEncoder::META_ALBUM_COMMENT, m_cddbEntry.get( KCDDB::Comment ).toString() );
                d->encoder->setMetaData( K3b::AudioEncoder::META_YEAR, QString::number(m_cddbEntry.get( KCDDB::Year ).toInt()) );
                d->encoder->setMetaData( K3b::AudioEncoder::META_GENRE, m_cddbEntry.get( KCDDB::Genre ).toString() );
            }
            else
                emit infoMessage( d->encoder->lastErrorString(), K3b::Job::MessageError );
        }
        else {
            isOpen = d->waveFileWriter->open( filename );
        }

        if( !isOpen ) {
            d->paranoiaLib->close();
            emit infoMessage( i18n("Unable to open '%1' for writing.", filename), K3b::Job::MessageError );
            m_device->block(false);
            return false;
        }

        emit infoMessage( i18n("Ripping to single file '%1'.", filename), K3b::Job::MessageInfo );
    }

    emit infoMessage( i18n("Starting digital audio extraction (ripping)."), K3b::Job::MessageInfo );

    bool success = true;
    for( int i = 0; i < m_tracks.count(); ++i ) {
        d->currentTrackIndex = i;
        if( !ripTrack( m_tracks[i].first, m_singleFile ? m_tracks[0].second : m_tracks[i].second ) ) {
            success = false;
            break;
        }
    }

    if( m_singleFile ) {
        if( d->encoder )
            d->encoder->closeFile();
        else
            d->waveFileWriter->close();

        if( success && !canceled() ) {
            QString& filename = m_tracks[0].second;
            emit infoMessage( i18n("Successfully ripped to %1.", filename), K3b::Job::MessageInfo );
        }
    }

    if( !canceled() && m_writePlaylist ) {
        success = success && writePlaylist();
    }

    if( !canceled() && m_writeCueFile && m_singleFile ) {
        if( !m_useIndex0 ) {
            emit newSubTask( i18n("Searching index 0 for all tracks") );
            m_device->indexScan( d->toc );
        }
        success = success && writeCueFile();
    }

    d->paranoiaLib->close();
    m_device->block(false);

    if( canceled() ) {
        return false;
    }
    else {
        return success;
    }
}


bool K3b::AudioRipJob::ripTrack( int track, const QString& filename )
{
    const K3b::Device::Track& tt = d->toc[track-1];

    long endSec = ( (m_useIndex0 && tt.index0() > 0)
                    ? tt.firstSector().lba() + tt.index0().lba() - 1
                    : tt.lastSector().lba() );

    if( d->paranoiaLib->initReading( tt.firstSector().lba(), endSec ) ) {

        long trackSectorsRead = 0;

        QString dir = filename.left( filename.lastIndexOf('/') );
        if( !KStandardDirs::makeDir( dir, 0777 ) ) {
            emit infoMessage( i18n("Unable to create folder %1", dir), K3b::Job::MessageError );
            return false;
        }

        // initialize
        bool isOpen = true;
        if( !m_singleFile ) {
            if( d->encoder ) {
                isOpen = d->encoder->openFile( d->fileType,
                                               filename,
                                               m_useIndex0 ? d->toc[track-1].realAudioLength() : d->toc[track-1].length() );
                if( isOpen ) {
                    d->encoder->setMetaData( K3b::AudioEncoder::META_TRACK_ARTIST, m_cddbEntry.track( track-1 ).get( KCDDB::Artist ).toString() );
                    d->encoder->setMetaData( K3b::AudioEncoder::META_TRACK_TITLE, m_cddbEntry.track( track-1 ).get( KCDDB::Title ).toString() );
                    d->encoder->setMetaData( K3b::AudioEncoder::META_TRACK_COMMENT, m_cddbEntry.track( track-1 ).get( KCDDB::Comment ).toString() );
                    d->encoder->setMetaData( K3b::AudioEncoder::META_TRACK_NUMBER, QString::number(track).rightJustified( 2, '0' ) );
                    d->encoder->setMetaData( K3b::AudioEncoder::META_ALBUM_ARTIST, m_cddbEntry.get( KCDDB::Artist ).toString() );
                    d->encoder->setMetaData( K3b::AudioEncoder::META_ALBUM_TITLE, m_cddbEntry.get( KCDDB::Title ).toString() );
                    d->encoder->setMetaData( K3b::AudioEncoder::META_ALBUM_COMMENT, m_cddbEntry.get( KCDDB::Comment ).toString() );
                    d->encoder->setMetaData( K3b::AudioEncoder::META_YEAR, QString::number(m_cddbEntry.get( KCDDB::Year ).toInt()) );
                    d->encoder->setMetaData( K3b::AudioEncoder::META_GENRE, m_cddbEntry.get( KCDDB::Genre ).toString() );
                }
                else
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

        if( !m_cddbEntry.track( track-1 ).get( KCDDB::Artist ).toString().isEmpty() &&
            !m_cddbEntry.track( track-1 ).get( KCDDB::Title ).toString().isEmpty() )
            emit newSubTask( i18n( "Ripping track %1 (%2 - %3)",
                                   track,
                                   m_cddbEntry.track( track-1 ).get( KCDDB::Artist ).toString(),
                                   m_cddbEntry.track( track-1 ).get( KCDDB::Title ).toString() ) );
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
                    if( m_singleFile )
                        emit infoMessage( i18n("Successfully ripped track %1.", track), K3b::Job::MessageInfo );
                    else
                        emit infoMessage( i18n("Successfully ripped track %1 to %2.", track, filename), K3b::Job::MessageInfo );

                    if( !m_singleFile ) {
                        if( d->encoder )
                            d->encoder->closeFile();
                        else
                            d->waveFileWriter->close();
                    }

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
    QString playlistDir = m_playlistFilename.left( m_playlistFilename.lastIndexOf( '/' ) );

    if( !KStandardDirs::makeDir( playlistDir ) ) {
        emit infoMessage( i18n("Unable to create folder %1",playlistDir), K3b::Job::MessageError );
        return false;
    }

    emit infoMessage( i18n("Writing playlist to %1.", m_playlistFilename ), K3b::Job::MessageInfo );

    QFile f( m_playlistFilename );
    if( f.open( QIODevice::WriteOnly ) ) {
        QTextStream t( &f );

        // format descriptor
        t << "#EXTM3U" << endl;

        // now write the entries (or the entry if m_singleFile)
        if( m_singleFile ) {
            // extra info
            t << "#EXTINF:" << d->overallSectorsToRead/75 << ",";
            if( !m_cddbEntry.get( KCDDB::Artist ).toString().isEmpty() &&
                !m_cddbEntry.get( KCDDB::Title ).toString().isEmpty() ) {
                t << m_cddbEntry.get( KCDDB::Artist ).toString() << " - " << m_cddbEntry.get( KCDDB::Title ).toString() << endl;
            }
            else {
                t << m_tracks[0].second.mid(m_tracks[0].second.lastIndexOf('/') + 1,
                                            m_tracks[0].second.length() - m_tracks[0].second.lastIndexOf('/') - 5)
                  << endl; // filename without extension
            }

            // filename
            if( m_relativePathInPlaylist )
                t << findRelativePath( m_tracks[0].second, playlistDir )
                  << endl;
            else
                t << m_tracks[0].second << endl;
        }
        else {
            for( int i = 0; i < m_tracks.count(); ++i ) {
                int trackIndex = m_tracks[i].first-1;

                // extra info
                t << "#EXTINF:" << d->toc[trackIndex].length().totalFrames()/75 << ",";

                if( !m_cddbEntry.track( trackIndex ).get( KCDDB::Artist ).toString().isEmpty() &&
                    !m_cddbEntry.track( trackIndex ).get( KCDDB::Title ).toString().isEmpty() ) {
                    t << m_cddbEntry.track( trackIndex ).get( KCDDB::Artist ).toString()
                      << " - "
                      << m_cddbEntry.track( trackIndex ).get( KCDDB::Title ).toString()
                      << endl;
                }
                else {
                    t << m_tracks[i].second.mid(m_tracks[i].second.lastIndexOf('/') + 1,
                                                m_tracks[i].second.length()
                                                - m_tracks[i].second.lastIndexOf('/') - 5)
                      << endl; // filename without extension
                }

                // filename
                if( m_relativePathInPlaylist )
                    t << findRelativePath( m_tracks[i].second, playlistDir )
                      << endl;
                else
                    t << m_tracks[i].second << endl;
            }
        }

        return ( t.status() == QTextStream::Ok );
    }
    else {
        emit infoMessage( i18n("Unable to open '%1' for writing.",m_playlistFilename), K3b::Job::MessageError );
        kDebug() << "(K3b::AudioRipJob) could not open file " << m_playlistFilename << " for writing.";
        return false;
    }
}


bool K3b::AudioRipJob::writeCueFile()
{
    K3b::CueFileWriter cueWriter;

    // create a new toc and cd-text
    K3b::Device::Toc toc;
    K3b::Device::CdText text;
    text.setPerformer( m_cddbEntry.get( KCDDB::Artist ).toString() );
    text.setTitle( m_cddbEntry.get( KCDDB::Title ).toString() );
    K3b::Msf currentSector;
    for( int i = 0; i < m_tracks.count(); ++i ) {
        int trackNum = m_tracks[i].first;

        const K3b::Device::Track& oldTrack = d->toc[trackNum-1];
        K3b::Device::Track newTrack( oldTrack );
        newTrack.setFirstSector( currentSector );
        newTrack.setLastSector( (currentSector+=oldTrack.length()) - 1 );
        toc.append( newTrack );

        text[i].setPerformer( m_cddbEntry.track( trackNum-1 ).get( KCDDB::Artist ).toString() );
        text[i].setTitle( m_cddbEntry.track( trackNum-1 ).get( KCDDB::Title ).toString() );
    }

    cueWriter.setData( toc );
    cueWriter.setCdText( text );


    // we always use a relative filename here
    QString imageFile = m_tracks[0].second.section( '/', -1 );
    cueWriter.setImage( imageFile, ( d->fileType.isEmpty() ? QString("WAVE") : d->fileType ) );

    // use the same base name as the image file
    QString cueFile = m_tracks[0].second;
    cueFile.truncate( cueFile.lastIndexOf('.') );
    cueFile += ".cue";

    emit infoMessage( i18n("Writing cue file to %1.",cueFile), K3b::Job::MessageInfo );

    return cueWriter.save( cueFile );
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
    if( m_cddbEntry.get( KCDDB::Title ).toString().isEmpty() )
        return i18n( "Ripping Audio Tracks" );
    else
        return i18n( "Ripping Audio Tracks From '%1'",
                     m_cddbEntry.get( KCDDB::Title ).toString() );
}

QString K3b::AudioRipJob::jobDetails() const
{
    if( d->encoder )
        return i18np( "1 track (encoding to %2)",
                      "%1 tracks (encoding to %2)",
                      m_tracks.count(),
                      d->encoder->fileTypeComment(d->fileType) );
    else
        return i18np( "1 track", "%1 tracks",
                      m_tracks.count() );
}

#include "k3baudioripjob.moc"
