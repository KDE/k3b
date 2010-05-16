/*
 *
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
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
#include "k3bffmpegwrapper.h"

#include <config-k3b.h>

extern "C" {
/*
 Recent versions of FFmepg uses C99 constant macros which are not presebt in C++ standard.
 The macro __STDC_CONSTANT_MACROS allow C++ to use these macros. Altough it's not defined by C++ standard
 it's supported by many implementations.
 See bug 236036 and discussion: http://lists.mplayerhq.hu/pipermail/ffmpeg-devel/2010-May/088074.html
 */
#define __STDC_CONSTANT_MACROS
#ifdef NEWFFMPEGAVCODECPATH
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#else
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#endif
}

#include <string.h>

#include <klocale.h>


#if LIBAVFORMAT_BUILD < 4629
#define FFMPEG_BUILD_PRE_4629
#endif


K3bFFMpegWrapper* K3bFFMpegWrapper::s_instance = 0;


class K3bFFMpegFile::Private
{
public:
    ::AVFormatContext* formatContext;
    ::AVCodec* codec;

    K3b::Msf length;

    // for decoding
    char outputBuffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    char* outputBufferPos;
    int outputBufferSize;
    ::AVPacket packet;
    quint8* packetData;
    int packetSize;
};


K3bFFMpegFile::K3bFFMpegFile( const QString& filename )
    : m_filename(filename)
{
    d = new Private;
    d->formatContext = 0;
    d->codec = 0;
}


K3bFFMpegFile::~K3bFFMpegFile()
{
    close();
    delete d;
}


bool K3bFFMpegFile::open()
{
    close();

    // open the file
    int err = ::av_open_input_file( &d->formatContext, m_filename.toLocal8Bit(), 0, 0, 0 );
    if( err < 0 ) {
        kDebug() << "(K3bFFMpegFile) unable to open " << m_filename << " with error " << err;
        return false;
    }

    // analyze the streams
    ::av_find_stream_info( d->formatContext );

    // we only handle files containing one audio stream
    if( d->formatContext->nb_streams != 1 ) {
        kDebug() << "(K3bFFMpegFile) more than one stream in " << m_filename;
        return false;
    }

    // urgh... ugly
#ifdef FFMPEG_BUILD_PRE_4629
    ::AVCodecContext* codecContext =  &d->formatContext->streams[0]->codec;
#else
    ::AVCodecContext* codecContext =  d->formatContext->streams[0]->codec;
#endif
    if( codecContext->codec_type != CODEC_TYPE_AUDIO ) {
        kDebug() << "(K3bFFMpegFile) not a simple audio stream: " << m_filename;
        return false;
    }

    // get the codec
    d->codec = ::avcodec_find_decoder(codecContext->codec_id);
    if( !d->codec ) {
        kDebug() << "(K3bFFMpegFile) no codec found for " << m_filename;
        return false;
    }

    // open the codec on our context
    kDebug() << "(K3bFFMpegFile) found codec for " << m_filename;
    if( ::avcodec_open( codecContext, d->codec ) < 0 ) {
        kDebug() << "(K3bFFMpegDecoderFactory) could not open codec.";
        return false;
    }

    // determine the length of the stream
    d->length = K3b::Msf::fromSeconds( (double)d->formatContext->duration / (double)AV_TIME_BASE );

    if( d->length == 0 ) {
        kDebug() << "(K3bFFMpegDecoderFactory) invalid length.";
        return false;
    }

    // dump some debugging info
    ::dump_format( d->formatContext, 0, m_filename.toLocal8Bit(), 0 );

    return true;
}


void K3bFFMpegFile::close()
{
    d->outputBufferSize = 0;
    d->packetSize = 0;
    d->packetData = 0;

    if( d->codec ) {
#ifdef FFMPEG_BUILD_PRE_4629
        ::avcodec_close( &d->formatContext->streams[0]->codec );
#else
        ::avcodec_close( d->formatContext->streams[0]->codec );
#endif
        d->codec = 0;
    }

    if( d->formatContext ) {
        ::av_close_input_file( d->formatContext );
        d->formatContext = 0;
    }
}


K3b::Msf K3bFFMpegFile::length() const
{
    return d->length;
}


int K3bFFMpegFile::sampleRate() const
{
#ifdef FFMPEG_BUILD_PRE_4629
    return d->formatContext->streams[0]->codec.sample_rate;
#else
    return d->formatContext->streams[0]->codec->sample_rate;
#endif
}


int K3bFFMpegFile::channels() const
{
#ifdef FFMPEG_BUILD_PRE_4629
    return d->formatContext->streams[0]->codec.channels;
#else
    return d->formatContext->streams[0]->codec->channels;
#endif
}


int K3bFFMpegFile::type() const
{
#ifdef FFMPEG_BUILD_PRE_4629
    return d->formatContext->streams[0]->codec.codec_id;
#else
    return d->formatContext->streams[0]->codec->codec_id;
#endif
}


QString K3bFFMpegFile::typeComment() const
{
    switch( type() ) {
    case CODEC_ID_WMAV1:
        return i18n("Windows Media v1");
    case CODEC_ID_WMAV2:
        return i18n("Windows Media v2");
#if LIBAVCODEC_VERSION_MAJOR < 52
    case CODEC_ID_MP3LAME:
#else
    case CODEC_ID_MP3:
#endif
        return i18n("MPEG 1 Layer III");
    case CODEC_ID_AAC:
        return i18n("Advanced Audio Coding (AAC)");
    default:
        return QString::fromLocal8Bit( d->codec->name );
    }
}


QString K3bFFMpegFile::title() const
{
    // FIXME: is this UTF8 or something??
    if( d->formatContext->title[0] != '\0' )
        return QString::fromLocal8Bit( d->formatContext->title );
    else
        return QString();
}


QString K3bFFMpegFile::author() const
{
    // FIXME: is this UTF8 or something??
    if( d->formatContext->author[0] != '\0' )
        return QString::fromLocal8Bit( d->formatContext->author );
    else
        return QString();
}


QString K3bFFMpegFile::comment() const
{
    // FIXME: is this UTF8 or something??
    if( d->formatContext->comment[0] != '\0' )
        return QString::fromLocal8Bit( d->formatContext->comment );
    else
        return QString();
}


int K3bFFMpegFile::read( char* buf, int bufLen )
{
    if( fillOutputBuffer() > 0 ) {
        int len = qMin(bufLen, d->outputBufferSize);
        ::memcpy( buf, d->outputBufferPos, len );

        // TODO: only swap if needed
        for( int i = 0; i < len-1; i+=2 ) {
            char a = buf[i];
            buf[i] = buf[i+1];
            buf[i+1] = a;
        }

        d->outputBufferPos += len;
        d->outputBufferSize -= len;
        return len;
    }
    else
        return 0;
}


// fill d->packetData with data to decode
int K3bFFMpegFile::readPacket()
{
    if( d->packetSize <= 0 ) {
        ::av_init_packet( &d->packet );

        if( ::av_read_frame( d->formatContext, &d->packet ) < 0 ) {
            return 0;
        }

        d->packetSize = d->packet.size;
        d->packetData = d->packet.data;
    }

    return d->packetSize;
}


// decode data in d->packetData and fill d->outputBuffer
int K3bFFMpegFile::fillOutputBuffer()
{
    // decode if the output buffer is empty
    if( d->outputBufferSize <= 0 ) {

        // make sure we have data to decode
        if( readPacket() == 0 ) {
            return 0;
        }

        d->outputBufferPos = d->outputBuffer;

#if LIBAVCODEC_VERSION_MAJOR < 52
        int len = ::avcodec_decode_audio(
#else
        int len = ::avcodec_decode_audio2(
#endif
#ifdef FFMPEG_BUILD_PRE_4629
            &d->formatContext->streams[0]->codec,
#else
            d->formatContext->streams[0]->codec,
#endif
            (short*)d->outputBuffer,
            &d->outputBufferSize,
            d->packetData, d->packetSize );

        d->packetSize -= len;
        d->packetData += len;

        if( d->packetSize <= 0  )
            ::av_free_packet( &d->packet );
    }

    // if it is still empty try again
    if( d->outputBufferSize <= 0 )
        return fillOutputBuffer();
    else
        return d->outputBufferSize;
}


bool K3bFFMpegFile::seek( const K3b::Msf& msf )
{
    d->outputBufferSize = 0;
    d->packetSize = 0;

    double seconds = (double)msf.totalFrames()/75.0;
    quint64 timestamp = (quint64)(seconds * (double)AV_TIME_BASE);

    // FIXME: do we really need the start_time and why?
#if LIBAVFORMAT_BUILD >= 4619
    return ( ::av_seek_frame( d->formatContext, -1, timestamp + d->formatContext->start_time, 0 ) >= 0 );
#else
    return ( ::av_seek_frame( d->formatContext, -1, timestamp + d->formatContext->start_time ) >= 0 );
#endif
}






K3bFFMpegWrapper::K3bFFMpegWrapper()
{
    ::av_register_all();
}


K3bFFMpegWrapper::~K3bFFMpegWrapper()
{
    s_instance = 0;
}


K3bFFMpegWrapper* K3bFFMpegWrapper::instance()
{
    if( !s_instance ) {
        s_instance = new K3bFFMpegWrapper();
    }

    return s_instance;
}


K3bFFMpegFile* K3bFFMpegWrapper::open( const QString& filename ) const
{
    K3bFFMpegFile* file = new K3bFFMpegFile( filename );
    if( file->open() ) {
#ifndef K3B_FFMPEG_ALL_CODECS
        //
        // only allow tested formats. ffmpeg seems not to be too reliable with every format.
        // mp3 being one of them sadly. Most importantly: allow the libsndfile decoder to do
        // its thing.
        //
        if( file->type() == CODEC_ID_WMAV1 ||
            file->type() == CODEC_ID_WMAV2 ||
            file->type() == CODEC_ID_AAC )
#endif
            return file;
    }

    delete file;
    return 0;
}
