/*
 *
 * $Id$
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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
#include "k3boggvorbisdecoder.h"

#include <config-k3b.h>

#include <qfile.h>
#include <qstringlist.h>

#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>

#include <stdio.h>
#include <stdlib.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>



class K3bOggVorbisDecoder::Private
{
public:
    Private()
        : vInfo(0),
          vComment(0),
          isOpen(false) {
    }

    OggVorbis_File oggVorbisFile;
    vorbis_info* vInfo;
    vorbis_comment* vComment;
    bool isOpen;
};


K3bOggVorbisDecoder::K3bOggVorbisDecoder( QObject* parent )
    : K3b::AudioDecoder( parent )
{
    d = new Private();
}


K3bOggVorbisDecoder::~K3bOggVorbisDecoder()
{
    delete d;
}


bool K3bOggVorbisDecoder::openOggVorbisFile()
{
    if( !d->isOpen ) {
        FILE* file = fopen( QFile::encodeName(filename()), "r" );
        if( !file ) {
            kDebug() << "(K3bOggVorbisDecoder) Could not open file " << filename();
            return false;
        }
        else if( ov_open( file, &d->oggVorbisFile, 0, 0 ) ) {
            kDebug() << "(K3bOggVorbisDecoder) " << filename()
                     << " seems not to to be an ogg vorbis file." << endl;
            fclose( file );
            return false;
        }
    }

    d->isOpen = true;
    return true;
}


bool K3bOggVorbisDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
    cleanup();

    if( openOggVorbisFile() ) {
        // check length of track
        double seconds = ov_time_total( &d->oggVorbisFile, -1 );
        if( seconds == OV_EINVAL ) {
            kDebug() << "(K3bOggVorbisDecoder) Could not determine length of file " << filename();
            cleanup();
            return false;
        }
        else {

            d->vInfo = ov_info( &d->oggVorbisFile, -1 /* current bitstream */ );
            d->vComment = ov_comment( &d->oggVorbisFile, -1 );

            // add meta tags
            for( int i = 0; i < d->vComment->comments; ++i ) {
                QString comment = QString::fromUtf8( d->vComment->user_comments[i] );
                QStringList values = comment.split( '=' );
                if( values.count() > 1 ) {
                    if( values[0].toLower() == "title" )
                        addMetaInfo( META_TITLE, values[1] );
                    else if( values[0].toLower() == "artist" )
                        addMetaInfo( META_ARTIST, values[1] );
                    else if( values[0].toLower() == "description" )
                        addMetaInfo( META_COMMENT, values[1] );
                }
            }


            // add technical infos
            addTechnicalInfo( i18n("Version"), QString::number(d->vInfo->version) );
            addTechnicalInfo( i18n("Channels"), QString::number(d->vInfo->channels) );
            addTechnicalInfo( i18n("Sampling Rate"), i18n("%1 Hz",d->vInfo->rate) );
            if( d->vInfo->bitrate_upper > 0 )
                addTechnicalInfo( i18n("Bitrate Upper"), i18n( "%1 bps" ,d->vInfo->bitrate_upper) );
            if( d->vInfo->bitrate_nominal > 0 )
                addTechnicalInfo( i18n("Bitrate Nominal"), i18n( "%1 bps" ,d->vInfo->bitrate_nominal) );
            if( d->vInfo->bitrate_lower > 0 )
                addTechnicalInfo( i18n("Bitrate Lower"), i18n( "%1 bps",d->vInfo->bitrate_lower) );

            frames = K3b::Msf::fromSeconds(seconds);
            samplerate = d->vInfo->rate;
            ch = d->vInfo->channels;

            cleanup();

            return true;
        }
    }
    else
        return false;
}


bool K3bOggVorbisDecoder::initDecoderInternal()
{
    cleanup();
    return openOggVorbisFile();
}


int K3bOggVorbisDecoder::decodeInternal( char* data, int maxLen )
{
    int bitStream = 0;
    long bytesRead = ov_read( &d->oggVorbisFile,
                              data,
                              maxLen,  // max length to be read
                              1,                   // big endian
                              2,                   // word size: 16-bit samples
                              1,                   // signed
                              &bitStream );        // current bitstream

    if( bitStream != 0 ) {
        kDebug() << "(K3bOggVorbisDecoder) bitstream != 0. Multiple bitstreams not supported.";
        return -1;
    }

    else if( bytesRead == OV_HOLE ) {
        kDebug() << "(K3bOggVorbisDecoder) OV_HOLE";
        // recursive new try
        return decodeInternal( data, maxLen );
    }

    else if( bytesRead < 0 ) {
        kDebug() << "(K3bOggVorbisDecoder) Error: " << bytesRead;
        return -1;
    }

    else if( bytesRead == 0 ) {
        kDebug() << "(K3bOggVorbisDecoder) successfully finished decoding.";
        return 0;
    }

    else {
        return bytesRead;
    }
}


void K3bOggVorbisDecoder::cleanup()
{
    if( d->isOpen )
        ov_clear( &d->oggVorbisFile );
    d->isOpen = false;
    d->vComment = 0;
    d->vInfo = 0;
}


bool K3bOggVorbisDecoder::seekInternal( const K3b::Msf& pos )
{
    return ( ov_pcm_seek( &d->oggVorbisFile, pos.pcmSamples() ) == 0 );
}


QString K3bOggVorbisDecoder::fileType() const
{
    return i18n("Ogg-Vorbis");
}


K3bOggVorbisDecoderFactory::K3bOggVorbisDecoderFactory( QObject* parent, const QVariantList& )
    : K3b::AudioDecoderFactory( parent )
{
}


K3bOggVorbisDecoderFactory::~K3bOggVorbisDecoderFactory()
{
}


K3b::AudioDecoder* K3bOggVorbisDecoderFactory::createDecoder( QObject* parent ) const
{
    return new K3bOggVorbisDecoder( parent );
}


bool K3bOggVorbisDecoderFactory::canDecode( const KUrl& url )
{
    FILE* file = fopen( QFile::encodeName(url.path()), "r" );
    if( !file ) {
        kDebug() << "(K3bOggVorbisDecoder) Could not open file " << url.path();
        return false;
    }

    OggVorbis_File of;

    if( ov_open( file, &of, 0, 0 ) ) {
        fclose( file );
        kDebug() << "(K3bOggVorbisDecoder) not an Ogg-Vorbis file: " << url.path();
        return false;
    }

    ov_clear( &of );

    return true;
}


#include "k3boggvorbisdecoder.moc"
