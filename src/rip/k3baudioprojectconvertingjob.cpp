/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2010 Michal Malek <michalm@jabster.pl>
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


#include "k3baudioprojectconvertingjob.h"

#include "k3bjob.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackreader.h"
#include "k3baudioencoder.h"
#include "k3bcuefilewriter.h"
#include "k3bglobals.h"
#include "k3bpatternparser.h"
#include "k3bwavefilewriter.h"

#include <libkcddb/cdinfo.h>

#include <KDebug>
#include <KStandardDirs>
#include <KLocale>

#include <QFile>
#include <QTimer>


class K3b::AudioProjectConvertingJob::Private
{
public:
    Private( AudioDoc* d )
        : doc(d),
          encoder(0),
          waveFileWriter(0),
          relativePathInPlaylist(false),
          writeCueFile(false),
          canceled(false) {
    }

    AudioDoc* doc;

    AudioEncoder* encoder;
    WaveFileWriter* waveFileWriter;

    KCDDB::CDInfo cddbEntry;

    Tracks tracks;
    QHash<QString,Msf> lengths;
    qint64 overallBytesRead;
    qint64 overallBytesToRead;

    QString playlistFilename;
    bool relativePathInPlaylist;
    bool writeCueFile;

    bool canceled;

    QString fileType;
};


K3b::AudioProjectConvertingJob::AudioProjectConvertingJob( K3b::AudioDoc* doc, K3b::JobHandler* hdl, QObject* parent )
    : K3b::ThreadJob( hdl,  parent ),
      d( new Private( doc ) )
{
}


K3b::AudioProjectConvertingJob::~AudioProjectConvertingJob()
{
    delete d->waveFileWriter;
    delete d;
}


void K3b::AudioProjectConvertingJob::setCddbEntry( const KCDDB::CDInfo& cddbEntry )
{
    d->cddbEntry = cddbEntry;
}


void K3b::AudioProjectConvertingJob::setEncoder( K3b::AudioEncoder* encoder )
{
    d->encoder = encoder;
}


void K3b::AudioProjectConvertingJob::setFileType( const QString& fileType )
{
    d->fileType = fileType;
}


void K3b::AudioProjectConvertingJob::setTracksToRip( const Tracks& tracks )
{
    d->tracks = tracks;
}


void K3b::AudioProjectConvertingJob::setWritePlaylist( const QString& filename, bool relativePaths )
{
    d->playlistFilename = filename;
    d->relativePathInPlaylist = relativePaths;
}


void K3b::AudioProjectConvertingJob::setWriteCueFile( bool b )
{
    d->writeCueFile = b;
}


bool K3b::AudioProjectConvertingJob::run()
{
    emit newTask( i18n("Converting Audio Tracks")  );

    if( !d->encoder )
        if( !d->waveFileWriter )
            d->waveFileWriter = new K3b::WaveFileWriter();

    d->overallBytesRead = 0;
    d->overallBytesToRead = d->doc->length().audioBytes();
    d->lengths.clear();

    Q_FOREACH( const QString& filename, d->tracks.keys().toSet() ) {
        d->lengths.insert( filename, 0 );
        Q_FOREACH( int trackNumber, d->tracks.values( filename ) ) {
            if( AudioTrack* track = d->doc->getTrack( trackNumber ) )
                d->lengths[ filename ] += track->length();
        }
    }

    emit infoMessage( i18n("Starting audio conversion."), K3b::Job::MessageInfo );

    bool success = true;
    QString lastFilename;
    Tracks::const_iterator track;
    for( track = d->tracks.constBegin(); success && track != d->tracks.constEnd(); ++track ) {
        if( AudioTrack* audioTrack = d->doc->getTrack( track.value() ) ) {
            success = convertTrack( *audioTrack, track.key(), lastFilename );
        }
        lastFilename = track.key();
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
        if( track != d->tracks.constEnd() ) {
            if( QFile::exists( track.key() ) ) {
                QFile::remove( track.key() );
                emit infoMessage( i18n("Removed partial file '%1'.", track.key()), K3b::Job::MessageInfo );
            }
        }

        return false;
    }
    else
        return success;
}


bool K3b::AudioProjectConvertingJob::convertTrack( AudioTrack& track, const QString& filename, const QString& prevFilename )
{
    QString dir = filename.left( filename.lastIndexOf('/') );
    if( !KStandardDirs::makeDir( dir ) ) {
        emit infoMessage( i18n("Unable to create folder %1",dir), K3b::Job::MessageError );
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
                metaData.insert( AudioEncoder::META_TRACK_NUMBER, QString::number(track.trackNumber()).rightJustified( 2, '0' ) );
                metaData.insert( AudioEncoder::META_TRACK_ARTIST, d->cddbEntry.track( track.trackNumber()-1 ).get( KCDDB::Artist ) );
                metaData.insert( AudioEncoder::META_TRACK_TITLE, d->cddbEntry.track( track.trackNumber()-1 ).get( KCDDB::Title ) );
                metaData.insert( AudioEncoder::META_TRACK_COMMENT, d->cddbEntry.track( track.trackNumber()-1 ).get( KCDDB::Comment ) );
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


    if( !d->cddbEntry.track( track.trackNumber()-1 ).get( KCDDB::Artist ).toString().isEmpty() &&
        !d->cddbEntry.track( track.trackNumber()-1 ).get( KCDDB::Title ).toString().isEmpty() )
        emit newSubTask( i18n( "Converting track %1 (%2 - %3)",
                               track.trackNumber(),
                               d->cddbEntry.track( track.trackNumber()-1 ).get( KCDDB::Artist ).toString(),
                               d->cddbEntry.track( track.trackNumber()-1 ).get( KCDDB::Title ).toString() ) );
    else
        emit newSubTask( i18n("Converting track %1", track.trackNumber()) );


    // do the conversion
    // ----------------------

    char buffer[10*1024];
    const qint64 bufferLength = 10LL*1024LL;
    qint64 readLength = 0;
    qint64 readFile = 0;

    AudioTrackReader trackReader( track );
    trackReader.open();

    while( !canceled() && !trackReader.atEnd() && ( readLength = trackReader.read( buffer, bufferLength ) ) > 0 ) {

        if( d->encoder ) {
            // the tracks produce big endian samples
            // so we need to swap the bytes here
            char b;
            for( qint64 i = 0; i < bufferLength-1; i+=2 ) {
                b = buffer[i];
                buffer[i] = buffer[i+1];
                buffer[i+1] = b;
            }

            if( d->encoder->encode( buffer, readLength ) < 0 ) {
                kDebug() << "(K3b::AudioProjectConvertingJob) error while encoding.";
                emit infoMessage( d->encoder->lastErrorString(), K3b::Job::MessageError );
                emit infoMessage( i18n("Error while encoding track %1.",track.trackNumber()), K3b::Job::MessageError );
                return false;
            }
        }
        else {
            d->waveFileWriter->write( buffer,
                                      readLength,
                                      K3b::WaveFileWriter::BigEndian );
        }

        d->overallBytesRead += readLength;
        readFile += readLength;
        emit subPercent( 100LL*readFile/trackReader.size() );
        emit percent( 100LL*d->overallBytesRead/d->overallBytesToRead );
    }

    emit infoMessage( i18n("Successfully converted track %1.", track.trackNumber()), K3b::Job::MessageInfo );
    return trackReader.atEnd();
}


bool K3b::AudioProjectConvertingJob::writePlaylist()
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

        Q_FOREACH( const QString& filename, d->tracks.keys().toSet() ) {

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
        kDebug() << "(K3b::AudioProjectConvertingJob) could not open file " << d->playlistFilename << " for writing.";
        return false;
    }
}


bool K3b::AudioProjectConvertingJob::writeCueFile()
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
            K3b::AudioTrack* oldTrack = d->doc->getTrack(trackNum);
            kDebug() << "creating track" << currentSector << ( currentSector+oldTrack->length() );

            K3b::Device::Track newTrack( currentSector, currentSector + oldTrack->length() - 1, K3b::Device::Track::TYPE_AUDIO );
            toc.append( newTrack );
            kDebug() << "created track" << newTrack;
            text[i].setPerformer( d->cddbEntry.track( i ).get( KCDDB::Artist ).toString() );
            text[i].setTitle( d->cddbEntry.track( i ).get( KCDDB::Title ).toString() );

            currentSector += oldTrack->length();
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


QString K3b::AudioProjectConvertingJob::findRelativePath( const QString& absPath, const QString& baseDir )
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


QString K3b::AudioProjectConvertingJob::jobDescription() const
{
    if( d->cddbEntry.get( KCDDB::Title ).toString().isEmpty() )
        return i18n( "Converting Audio Tracks" );
    else
        return i18n( "Converting Audio Tracks From '%1'",
                     d->cddbEntry.get( KCDDB::Title ).toString() );
}

QString K3b::AudioProjectConvertingJob::jobDetails() const
{
    if( d->encoder )
        return i18np( "1 track (encoding to %2)",
                      "%1 tracks (encoding to %2)",
                      d->tracks.count(),
                      d->encoder->fileTypeComment(d->fileType) );
    else
        return i18np( "1 track", "%1 tracks",
                      d->doc->numOfTracks() );
}

#include "k3baudioprojectconvertingjob.moc"
