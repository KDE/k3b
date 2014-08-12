/*
 *
 *
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

#include "k3bffmpegdecoder.h"
#include "k3bffmpegwrapper.h"

#include <config-k3b.h>

#include <QtCore/QDebug>

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
#else
#include <ffmpeg/avcodec.h>
#endif
}

#include <math.h>

K3B_EXPORT_PLUGIN(k3bffmpegdecoder, K3bFFMpegDecoderFactory)

K3bFFMpegDecoderFactory::K3bFFMpegDecoderFactory( QObject* parent, const QVariantList& )
    : K3b::AudioDecoderFactory( parent )
{
}


K3bFFMpegDecoderFactory::~K3bFFMpegDecoderFactory()
{
}


K3b::AudioDecoder* K3bFFMpegDecoderFactory::createDecoder( QObject* parent ) const
{
    return new K3bFFMpegDecoder( parent);
}


bool K3bFFMpegDecoderFactory::canDecode( const QUrl& url )
{
    K3bFFMpegFile* file = K3bFFMpegWrapper::instance()->open( url.toLocalFile() );
    if( file ) {
        delete file;
        return true;
    }
    else {
        return false;
    }
}






K3bFFMpegDecoder::K3bFFMpegDecoder( QObject* parent  )
    : K3b::AudioDecoder( parent ),
      m_file(0)
{
}


K3bFFMpegDecoder::~K3bFFMpegDecoder()
{
}


QString K3bFFMpegDecoder::fileType() const
{
    return m_type;
}


bool K3bFFMpegDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
    m_file = K3bFFMpegWrapper::instance()->open( filename() );
    if( m_file ) {

        // TODO: call addTechnicalInfo

        addMetaInfo( META_TITLE, m_file->title() );
        addMetaInfo( META_ARTIST, m_file->author() );
        addMetaInfo( META_COMMENT, m_file->comment() );

        samplerate = m_file->sampleRate();
        ch = m_file->channels();
        m_type = m_file->typeComment();
        frames = m_file->length();

        // ffmpeg's length information is not reliable at all
        // so we have to decode the whole file in order to get the correct length
//     char buffer[10*2048];
//     int len = 0;
//     unsigned long long bytes = 0;
//     while( ( len = m_file->read( buffer, 10*2048 ) ) > 0 )
//       bytes += len;

//     frames = (unsigned long)ceil((double)bytes/2048.0);

        // cleanup;
        delete m_file;
        m_file = 0;

        return true;
    }
    else
        return false;
}


bool K3bFFMpegDecoder::initDecoderInternal()
{
    if( !m_file )
        m_file = K3bFFMpegWrapper::instance()->open( filename() );

    return (m_file != 0);
}


void K3bFFMpegDecoder::cleanup()
{
    delete m_file;
    m_file = 0;
}


bool K3bFFMpegDecoder::seekInternal( const K3b::Msf& msf )
{
    if( msf == 0 )
        return initDecoderInternal();
    else
        return m_file->seek( msf );
}


int K3bFFMpegDecoder::decodeInternal( char* _data, int maxLen )
{
    return m_file->read( _data, maxLen );
}


#include "k3bffmpegdecoder.moc"
