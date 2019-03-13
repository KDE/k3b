/*
 *
 *  Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bmassaudioencodingjob.h"
#include "k3baudioencoder.h"
#include "k3bcuefilewriter.h"
#include "k3bwavefilewriter.h"

#include <KLocalizedString>
#include <KCddb/Cdinfo>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QTextStream>

#include <vector>
#include <algorithm>

namespace K3b {

namespace
{

    struct SortByTrackNumber
    {
        SortByTrackNumber( MassAudioEncodingJob::Tracks const& tracks ) : m_tracks( tracks ) {}

        bool operator()( QString const& lhs, QString const& rhs )
        {
            return m_tracks.value( lhs ) < m_tracks.value( rhs );
        }

        MassAudioEncodingJob::Tracks const& m_tracks;
    };

    struct Task {
        Task(MassAudioEncodingJob::Tracks::const_iterator const & it) :
            tracknumber(it.value()),
            filename   (it.key()),
            track      (it)
        { }

        static
        bool sort_by_filename(Task const & lhs, Task const & rhs)
        {
          return lhs.filename < rhs.filename;
        }

        static
        bool sort_by_tracknumber(Task const & lhs, Task const & rhs)
        {
          return lhs.tracknumber < rhs.tracknumber;
        }

        int     tracknumber;
        QString filename;
        MassAudioEncodingJob::Tracks::const_iterator track;
    };

} // namespace


class MassAudioEncodingJob::Private
{
public:
    Private( bool be )
    :
        bigEndian( be ),
        overallBytesRead( 0 ),
        overallBytesToRead( 0 ),
        encoder( 0 ),
        waveFileWriter( 0 ),
        relativePathInPlaylist( false ),
        writeCueFile( false )
    {
    }

    const bool bigEndian;
    Tracks tracks;
    QHash<QString,Msf> lengths;
    qint64 overallBytesRead;
    qint64 overallBytesToRead;
    AudioEncoder* encoder;
    WaveFileWriter* waveFileWriter;
    QString fileType;
    KCDDB::CDInfo cddbEntry;

    QString playlistFilename;
    bool relativePathInPlaylist;
    bool writeCueFile;
};


MassAudioEncodingJob::MassAudioEncodingJob( bool bigEndian, JobHandler* jobHandler, QObject* parent )
    : ThreadJob( jobHandler, parent ),
      d( new Private( bigEndian ) )
{
}


MassAudioEncodingJob::~MassAudioEncodingJob()
{
}


void MassAudioEncodingJob::setCddbEntry( const KCDDB::CDInfo& cddbEntry )
{
    d->cddbEntry = cddbEntry;
}


const KCDDB::CDInfo& MassAudioEncodingJob::cddbEntry() const
{
    return d->cddbEntry;
}


void MassAudioEncodingJob::setEncoder( AudioEncoder* encoder )
{
    d->encoder = encoder;
}


AudioEncoder* MassAudioEncodingJob::encoder() const
{
    return d->encoder;
}


void MassAudioEncodingJob::setFileType( const QString& fileType )
{
    d->fileType = fileType;
}



const QString& MassAudioEncodingJob::fileType() const
{
    return d->fileType;
}


void MassAudioEncodingJob::setTrackList( const Tracks& tracks )
{
    d->tracks = tracks;
}


const MassAudioEncodingJob::Tracks& MassAudioEncodingJob::trackList() const
{
    return d->tracks;
}


void MassAudioEncodingJob::setWritePlaylist( const QString& filename, bool relativePaths )
{
    d->playlistFilename = filename;
    d->relativePathInPlaylist = relativePaths;
}


void MassAudioEncodingJob::setWriteCueFile( bool writeCueFile )
{
    d->writeCueFile = writeCueFile;
}


QString MassAudioEncodingJob::jobDetails() const
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


QString MassAudioEncodingJob::jobTarget() const
{
    Tracks::const_iterator it = d->tracks.constBegin();
    if( it != d->tracks.constEnd() )
        return QFileInfo( it.key() ).absolutePath();
    else
        return QString();
}


bool MassAudioEncodingJob::init()
{
    return true;
}
        
        
void MassAudioEncodingJob::cleanup()
{
}


bool MassAudioEncodingJob::run()
{
    if ( !init() )
        return false;

    if( !d->encoder )
        if( !d->waveFileWriter )
            d->waveFileWriter = new K3b::WaveFileWriter();

    d->overallBytesRead = 0;
    d->overallBytesToRead = 0;
    d->lengths.clear();

    Q_FOREACH( const QString& filename, d->tracks.keys().toSet() ) {
        d->lengths.insert( filename, 0 );
        Q_FOREACH( int trackNumber, d->tracks.values( filename ) ) {
            const Msf length = trackLength( trackNumber );
            d->lengths[ filename ] += length;
            d->overallBytesToRead += length.audioBytes();
        }
    }

    // rip tracks in *numerical* order
    std::vector<Task> tasks;
    tasks.reserve( d->tracks.size() );
    for( Tracks::const_iterator i = d->tracks.constBegin(); i != d->tracks.constEnd(); ++i )
        tasks.push_back( Task(i) );
    std::sort( tasks.begin(), tasks.end(), Task::sort_by_tracknumber );

    bool success = true;
    QString lastFilename;
    std::vector<Task>::const_iterator currentTask;
    for( currentTask = tasks.begin(); success && currentTask != tasks.end(); ++currentTask ) {
        success = encodeTrack( currentTask->track.value(), currentTask->track.key(), lastFilename );
        lastFilename = currentTask->track.key();
    }

    if( d->encoder )
        d->encoder->closeFile();
    if( d->waveFileWriter )
        d->waveFileWriter->close();

    if( !canceled() && success && !d->playlistFilename.isNull() ) {
        success = success && writePlaylist();
    }

    if( !canceled() && success && d->writeCueFile ) {
        success = success && writeCueFile();
    }

    if( canceled() ) {
        if( currentTask != tasks.end() ) {
            if( QFile::exists( currentTask->track.key() ) ) {
                QFile::remove( currentTask->track.key() );
                emit infoMessage( i18n("Removed partial file '%1'.", currentTask->track.key()), K3b::Job::MessageInfo );
            }
        }

        success = false;
    }
    
    cleanup();
    return success;
}


bool K3b::MassAudioEncodingJob::encodeTrack( int trackIndex, const QString& filename, const QString& prevFilename )
{
    QScopedPointer<QIODevice> source( createReader( trackIndex ) );
    if( source.isNull() ) {
        return false;
    }
    
    QDir dir = QFileInfo( filename ).dir();
    if( !QDir().mkpath( dir.path() ) ) {
        emit infoMessage( i18n("Unable to create folder %1",dir.path()), K3b::Job::MessageError );
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
                metaData.insert( AudioEncoder::META_TRACK_NUMBER, QString::number(trackIndex).rightJustified( 2, '0' ) );
                metaData.insert( AudioEncoder::META_TRACK_ARTIST, d->cddbEntry.track( trackIndex-1 ).get( KCDDB::Artist ) );
                metaData.insert( AudioEncoder::META_TRACK_TITLE, d->cddbEntry.track( trackIndex-1 ).get( KCDDB::Title ) );
                metaData.insert( AudioEncoder::META_TRACK_COMMENT, d->cddbEntry.track( trackIndex-1 ).get( KCDDB::Comment ) );
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
            emit infoMessage( i18n("Unable to open '%1' for writing.",filename), K3b::Job::MessageError );
            return false;
        }
    }

    trackStarted( trackIndex );

    // do the conversion
    // ----------------------

    char buffer[10*1024];
    const qint64 bufferLength = 10LL*1024LL;
    qint64 readLength = 0;
    qint64 readFile = 0;

    if( !source->open( QIODevice::ReadOnly ) ) {
        emit infoMessage( source->errorString(), Job::MessageError );
        return false;
    }

    while( !canceled() && !source->atEnd() && ( readLength = source->read( buffer, bufferLength ) ) > 0 ) {

        if( d->encoder ) {

            if( d->bigEndian ) {
                // the tracks produce big endian samples
                // and encoder encoder consumes little endian
                // so we need to swap the bytes here
                char b;
                for( qint64 i = 0; i < bufferLength-1; i+=2 ) {
                    b = buffer[i];
                    buffer[i] = buffer[i+1];
                    buffer[i+1] = b;
                }
            }

            if( d->encoder->encode( buffer, readLength ) < 0 ) {
                qDebug() << "error while encoding.";
                emit infoMessage( d->encoder->lastErrorString(), K3b::Job::MessageError );
                emit infoMessage( i18n("Error while encoding track %1.",trackIndex), K3b::Job::MessageError );
                return false;
            }
        }
        else {
            d->waveFileWriter->write( buffer,
                                      readLength,
                                      d->bigEndian ? WaveFileWriter::BigEndian : WaveFileWriter::LittleEndian );
        }

        d->overallBytesRead += readLength;
        readFile += readLength;
        emit subPercent( 100LL*readFile/source->size() );
        emit percent( 100LL*d->overallBytesRead/d->overallBytesToRead );
    }

    if( !canceled() && !source->atEnd() ) {
        emit infoMessage( source->errorString(), Job::MessageError );
        return false;
    }

    trackFinished( trackIndex, filename );
    return source->atEnd();
}


bool MassAudioEncodingJob::writePlaylist()
{
    QFileInfo playlistInfo( d->playlistFilename );
    QDir playlistDir( playlistInfo.dir() );

    if( !QDir().mkpath( playlistDir.path() ) ) {
        emit infoMessage( i18n("Unable to create folder %1",playlistDir.path()), Job::MessageError );
        return false;
    }

    emit infoMessage( i18n("Writing playlist to %1.", d->playlistFilename ), Job::MessageInfo );

    QFile f( d->playlistFilename );
    if( f.open( QIODevice::WriteOnly ) ) {
        QTextStream t( &f );

        // format descriptor
        t << "#EXTM3U" << endl;

        // Get list of the ripped filenames sorted by track number
        QStringList filenames = d->tracks.keys();
        filenames.removeDuplicates();
        std::sort(filenames.begin(), filenames.end(), SortByTrackNumber(d->tracks));

        Q_FOREACH( const QString& trackFile, filenames ) {

            // extra info
            t << "#EXTINF:" << d->lengths[trackFile].totalFrames()/75 << ",";

            QVariant artist;
            QVariant title;

            QList<int> trackNums = d->tracks.values( trackFile );
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
                t << QFileInfo( trackFile ).baseName() << endl;
            }

            // filename
            if( d->relativePathInPlaylist )
                t << playlistDir.relativeFilePath( trackFile ) << endl;
            else
                t << trackFile << endl;
        }

        return ( t.status() == QTextStream::Ok );
    }
    else {
        emit infoMessage( i18n("Unable to open '%1' for writing.",d->playlistFilename), Job::MessageError );
        qDebug() << "could not open file " << d->playlistFilename << " for writing.";
        return false;
    }
}


bool MassAudioEncodingJob::writeCueFile()
{
    bool success = true;
    Q_FOREACH( const QString& filename, d->tracks.keys().toSet() ) {
        CueFileWriter cueWriter;

        // create a new toc and cd-text
        Device::Toc toc;
        Device::CdText text;
        text.setPerformer( d->cddbEntry.get( KCDDB::Artist ).toString() );
        text.setTitle( d->cddbEntry.get( KCDDB::Title ).toString() );
        Msf currentSector;

        QList<int> trackNums = d->tracks.values( filename );
        for( int i = 0; i < trackNums.size(); ++i ) {
            const int trackNum = trackNums[ i ];
            const Msf length = trackLength( trackNum );
            Device::Track newTrack( currentSector, currentSector + length - 1, Device::Track::TYPE_AUDIO );
            toc.append( newTrack );

            text.track(i).setPerformer( d->cddbEntry.track( trackNum-1 ).get( KCDDB::Artist ).toString() );
            text.track(i).setTitle( d->cddbEntry.track( trackNum-1 ).get( KCDDB::Title ).toString() );
            
            currentSector += length;
        }

        cueWriter.setData( toc );
        cueWriter.setCdText( text );
        
        QFileInfo fileInfo( filename );

        // we always use a relative filename here
        cueWriter.setImage( fileInfo.fileName(), ( d->fileType.isEmpty() ? QString("WAVE") : d->fileType ) );

        // use the same base name as the image file
        QString cueFile = fileInfo.dir().filePath( fileInfo.baseName() + ".cue" );

        emit infoMessage( i18n("Writing cue file to %1.",cueFile), Job::MessageInfo );

        success = success && cueWriter.save( cueFile );
    }
    return success;
}

} // namespace K3b


