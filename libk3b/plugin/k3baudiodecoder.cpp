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

#include <config-libk3b.h>

#include <k3bcore.h>

#include "k3baudiodecoder.h"
#include "k3bpluginmanager.h"

#include <kdebug.h>
#include <kfilemetainfo.h>

#include <qmap.h>

#include <math.h>

#include <samplerate.h>

#if !(HAVE_LRINT && HAVE_LRINTF)
#define lrint(dbl)              ((int) (dbl))
#define lrintf(flt)             ((int) (flt))
#endif

// use a one second buffer
static const int DECODING_BUFFER_SIZE = 75*2352;

class K3b::AudioDecoder::Private
{
public:
    Private()
        : metaInfo(0),
          resampleState(0),
          resampleData(0),
          inBuffer(0),
          inBufferPos(0),
          inBufferFill(0),
          outBuffer(0),
          monoBuffer(0),
          decodingBufferPos(0),
          decodingBufferFill(0),
          valid(true) {
    }

    // the current position of the decoder
    // This does NOT include the decodingBuffer
    K3b::Msf currentPos;

    // since the current position above is measured in frames
    // there might be a little offset since the decoded data is not
    // always a multiple of 2353 bytes
    int currentPosOffset;

    // already decoded bytes from last init or last seek
    // TODO: replace alreadyDecoded with currentPos
    unsigned long alreadyDecoded;

    K3b::Msf decodingStartPos;

    KFileMetaInfo* metaInfo;

    // set to true once decodeInternal() returned 0
    bool decoderFinished;

    // resampling
    SRC_STATE* resampleState;
    SRC_DATA* resampleData;

    float* inBuffer;
    float* inBufferPos;
    int inBufferFill;

    float* outBuffer;

    int samplerate;
    int channels;

    // mono -> stereo conversion
    char* monoBuffer;

    char decodingBuffer[DECODING_BUFFER_SIZE];
    char* decodingBufferPos;
    int decodingBufferFill;

    QMap<QString, QString> technicalInfoMap;
    QMap<MetaDataField, QString> metaInfoMap;

    bool valid;
};



K3b::AudioDecoder::AudioDecoder( QObject* parent )
    : QObject( parent )
{
    d = new Private();
}


K3b::AudioDecoder::~AudioDecoder()
{
    cleanup();

    if( d->inBuffer ) delete [] d->inBuffer;
    if( d->outBuffer ) delete [] d->outBuffer;
    if( d->monoBuffer ) delete [] d->monoBuffer;

    delete d->metaInfo;
    delete d->resampleData;
    if( d->resampleState )
        src_delete( d->resampleState );
    delete d;
}


void K3b::AudioDecoder::setFilename( const QString& filename )
{
    m_fileName = filename;
    delete d->metaInfo;
    d->metaInfo = 0;
}


bool K3b::AudioDecoder::isValid() const
{
    return d->valid;
}


bool K3b::AudioDecoder::analyseFile()
{
    d->technicalInfoMap.clear();
    d->metaInfoMap.clear();
    delete d->metaInfo;
    d->metaInfo = 0;

    cleanup();

    bool ret = analyseFileInternal( m_length, d->samplerate, d->channels );
    if( ret && ( d->channels == 1 || d->channels == 2 ) && m_length > 0 ) {
        d->valid = initDecoder();
        return d->valid;
    }
    else {
        d->valid = false;
        return false;
    }
}


bool K3b::AudioDecoder::initDecoder( const K3b::Msf& startOffset )
{
    if( initDecoder() ) {
        if( startOffset > 0 )
            return seek( startOffset );
        else
            return true;
    }
    else
        return false;
}


bool K3b::AudioDecoder::initDecoder()
{
    cleanup();

    if( d->resampleState )
        src_reset( d->resampleState );

    d->alreadyDecoded = 0;
    d->currentPos = 0;
    d->currentPosOffset = 0;
    d->decodingBufferFill = 0;
    d->decodingBufferPos = 0;
    d->decodingStartPos = 0;
    d->inBufferFill = 0;

    d->decoderFinished = false;

    return initDecoderInternal();
}


int K3b::AudioDecoder::decode( char* _data, int maxLen )
{
    unsigned long lengthToDecode = (m_length - d->decodingStartPos).audioBytes();

    if( d->alreadyDecoded >= lengthToDecode )
        return 0;

    if( maxLen <= 0 )
        return 0;

    int read = 0;

    if( d->decodingBufferFill == 0 ) {
        //
        // now we decode into the decoding buffer
        // to ensure a minimum buffer size
        //
        d->decodingBufferFill = 0;
        d->decodingBufferPos = d->decodingBuffer;

        if( !d->decoderFinished ) {
            if( d->samplerate != 44100 ) {

                // check if we have data left from some previous conversion
                if( d->inBufferFill > 0 ) {
                    read = resample( d->decodingBuffer, DECODING_BUFFER_SIZE );
                }
                else {
                    if( !d->inBuffer ) {
                        d->inBuffer = new float[DECODING_BUFFER_SIZE/2];
                    }

                    if( (read = decodeInternal( d->decodingBuffer, DECODING_BUFFER_SIZE )) == 0 )
                        d->decoderFinished = true;

                    d->inBufferFill = read/2;
                    d->inBufferPos = d->inBuffer;
                    from16bitBeSignedToFloat( d->decodingBuffer, d->inBuffer, d->inBufferFill );

                    read = resample( d->decodingBuffer, DECODING_BUFFER_SIZE );
                }
            }
            else if( d->channels == 1 ) {
                if( !d->monoBuffer ) {
                    d->monoBuffer = new char[DECODING_BUFFER_SIZE/2];
                }

                // we simply duplicate every frame
                if( (read = decodeInternal( d->monoBuffer, DECODING_BUFFER_SIZE/2 )) == 0 )
                    d->decoderFinished = true;

                for( int i = 0; i < read; i+=2 ) {
                    d->decodingBuffer[2*i] = d->decodingBuffer[2*i+2] = d->monoBuffer[i];
                    d->decodingBuffer[2*i+1] = d->decodingBuffer[2*i+3] = d->monoBuffer[i+1];
                }

                read *= 2;
            }
            else {
                if( (read = decodeInternal( d->decodingBuffer, DECODING_BUFFER_SIZE )) == 0 )
                    d->decoderFinished = true;
            }
        }

        if( read < 0 ) {
            return -1;
        }
        else if( read == 0 ) {
            // check if we need to pad
            int bytesToPad = lengthToDecode - d->alreadyDecoded;
            if( bytesToPad > 0 ) {
                kDebug() << "(K3b::AudioDecoder) track length: " << lengthToDecode
                         << "; decoded module data: " << d->alreadyDecoded
                         << "; we need to pad " << bytesToPad << " bytes." << endl;

                if( DECODING_BUFFER_SIZE < bytesToPad )
                    bytesToPad = DECODING_BUFFER_SIZE;

                ::memset( d->decodingBuffer, 0, bytesToPad );

                kDebug() << "(K3b::AudioDecoder) padded " << bytesToPad << " bytes.";

                read = bytesToPad;
            }
            else {
                kDebug() << "(K3b::AudioDecoder) decoded " << d->alreadyDecoded << " bytes.";
                return 0;
            }
        }
        else {

            // check if we decoded too much
            if( d->alreadyDecoded + read > lengthToDecode ) {
                kDebug() << "(K3b::AudioDecoder) we decoded too much. Cutting output by "
                         << (read + d->alreadyDecoded - lengthToDecode) << endl;
                read = lengthToDecode - d->alreadyDecoded;
            }
        }

        d->decodingBufferFill = read;
    }


    // clear out the decoding buffer
    read = qMin( maxLen, d->decodingBufferFill );
    ::memcpy( _data, d->decodingBufferPos, read );
    d->decodingBufferPos += read;
    d->decodingBufferFill -= read;

    d->alreadyDecoded += read;
    d->currentPos += (read+d->currentPosOffset)/2352;
    d->currentPosOffset = (read+d->currentPosOffset)%2352;

    return read;
}


// resample data in d->inBufferPos and save the result to data
//
//
int K3b::AudioDecoder::resample( char* data, int maxLen )
{
    if( !d->resampleState ) {
        d->resampleState = src_new( SRC_SINC_MEDIUM_QUALITY, d->channels, 0 );
        if( !d->resampleState ) {
            kDebug() << "(K3b::AudioDecoder) unable to initialize resampler.";
            return -1;
        }
        d->resampleData = new SRC_DATA;
    }

    if( !d->outBuffer ) {
        d->outBuffer = new float[DECODING_BUFFER_SIZE/2];
    }

    d->resampleData->data_in = d->inBufferPos;
    d->resampleData->data_out = d->outBuffer;
    d->resampleData->input_frames = d->inBufferFill/d->channels;
    d->resampleData->output_frames = maxLen/2/2;  // in case of mono files we need the space anyway
    d->resampleData->src_ratio = 44100.0/(double)d->samplerate;
    if( d->inBufferFill == 0 )
        d->resampleData->end_of_input = 1;  // this should force libsamplerate to output the last frames
    else
        d->resampleData->end_of_input = 0;

    int len = 0;
    if( (len = src_process( d->resampleState, d->resampleData ) ) ) {
        kDebug() << "(K3b::AudioDecoder) error while resampling: " << src_strerror(len);
        return -1;
    }

    if( d->channels == 2 )
        fromFloatTo16BitBeSigned( d->outBuffer, data, d->resampleData->output_frames_gen*d->channels );
    else {
        for( int i = 0; i < d->resampleData->output_frames_gen; ++i ) {
            fromFloatTo16BitBeSigned( &d->outBuffer[i], &data[4*i], 1 );
            fromFloatTo16BitBeSigned( &d->outBuffer[i], &data[4*i+2], 1 );
        }
    }

    d->inBufferPos += d->resampleData->input_frames_used*d->channels;
    d->inBufferFill -= d->resampleData->input_frames_used*d->channels;
    if( d->inBufferFill <= 0 ) {
        d->inBufferPos = d->inBuffer;
        d->inBufferFill = 0;
    }

    // 16 bit frames, so we need to multiply by 2
    // and we always have two channels
    return d->resampleData->output_frames_gen*2*2;
}


void K3b::AudioDecoder::from16bitBeSignedToFloat( char* src, float* dest, int samples )
{
    while( samples ) {
        samples--;
        dest[samples] = static_cast<float>( qint16(((src[2*samples]<<8)&0xff00)|(src[2*samples+1]&0x00ff)) / 32768.0 );
    }
}


void K3b::AudioDecoder::fromFloatTo16BitBeSigned( float* src, char* dest, int samples )
{
    while( samples ) {
        samples--;

        float scaled = src[samples] * 32768.0;
        qint16 val = 0;

        // clipping
        if( scaled >= ( 1.0 * 0x7FFF ) )
            val = 32767;
        else if( scaled <= ( -8.0 * 0x1000 ) )
            val = -32768;
        else
            val = lrintf(scaled);

        dest[2*samples]   = val>>8;
        dest[2*samples+1] = val;
    }
}


void K3b::AudioDecoder::from8BitTo16BitBeSigned( char* src, char* dest, int samples )
{
    while( samples ) {
        samples--;

        float scaled = static_cast<float>(quint8(src[samples])-128) / 128.0 * 32768.0;
        qint16 val = 0;

        // clipping
        if( scaled >= ( 1.0 * 0x7FFF ) )
            val = 32767;
        else if( scaled <= ( -8.0 * 0x1000 ) )
            val = -32768;
        else
            val = lrintf(scaled);

        dest[2*samples]   = val>>8;
        dest[2*samples+1] = val;
    }
}


bool K3b::AudioDecoder::seek( const K3b::Msf& pos )
{
    kDebug() << "(K3b::AudioDecoder) seek from " << d->currentPos.toString() << " (+" << d->currentPosOffset
             << ") to " << pos.toString() << endl;

    if( pos > length() )
        return false;

    d->decoderFinished = false;

    if( pos == d->currentPos && d->currentPosOffset == 0 )
        return true;

    if( pos == 0 )
        return initDecoder();

    bool success = false;

    //
    // First check if we may do a "perfect seek".
    // We cannot rely on the decoding plugins to seek perfectly. Especially
    // the mp3 decoder does not. But in case we want to split a live recording
    // it is absolutely nesseccary to perform a perfect seek.
    // So if we did not already decode past the seek position and the difference
    // between the current position and the seek position is less than some fixed
    // value we simply decode up to the seek position.
    //
    if( ( pos > d->currentPos ||
          ( pos == d->currentPos && d->currentPosOffset == 0 ) )
        &&
        ( pos - d->currentPos < K3b::Msf(0,10,0) ) ) {  // < 10 seconds is ok
        kDebug() << "(K3b::AudioDecoder) performing perfect seek from " << d->currentPos.toString()
                 << " to " << pos.toString() << ". :)" << endl;

        qint64 bytesToDecode = pos.audioBytes() - d->currentPos.audioBytes() - d->currentPosOffset;
        kDebug() << "(K3b::AudioDecoder) seeking " << bytesToDecode << " bytes.";
        char buffi[10*2352];
        while( bytesToDecode > 0 ) {
            int read = decode( buffi, qMin(( qint64 )( 10*2352 ), bytesToDecode) );
            if( read <= 0 )
                return false;

            bytesToDecode -= read;
        }

        kDebug() << "(K3b::AudioDecoder) perfect seek done.";

        success = true;
    }
    else {
        //
        // Here we have to reset the resampling stuff since we restart decoding at another position.
        //
        if( d->resampleState )
            src_reset( d->resampleState );
        d->inBufferFill = 0;

        //
        // And also reset the decoding buffer to not return any garbage from previous decoding.
        //
        d->decodingBufferFill = 0;

        success = seekInternal( pos );
    }

    d->alreadyDecoded = 0;
    d->currentPos = d->decodingStartPos = pos;
    d->currentPosOffset = 0;

    return success;
}


void K3b::AudioDecoder::cleanup()
{
}


QString K3b::AudioDecoder::metaInfo( MetaDataField f )
{
    if( d->metaInfoMap.contains( f ) )
        return d->metaInfoMap[f];

    // fall back to KFileMetaInfo
    if( !d->metaInfo )
        d->metaInfo = new KFileMetaInfo( filename() );

    if( d->metaInfo->isValid() ) {
        QString tag;
        switch( f ) {
        case META_TITLE:
            tag = "Title";
            break;
        case META_ARTIST:
            tag = "Artist";
            break;
        case META_SONGWRITER:
            tag = "Songwriter";
            break;
        case META_COMPOSER:
            tag = "Composer";
            break;
        case META_COMMENT:
            tag = "Comment";
            break;
        }

        KFileMetaInfoItem item = d->metaInfo->item( tag );
        if( item.isValid() )
            return item.value().toString();
    }

    return QString();
}


void K3b::AudioDecoder::addMetaInfo( MetaDataField f, const QString& value )
{
    if( !value.isEmpty() )
        d->metaInfoMap[f] = value;
    else
        kDebug() << "(K3b::AudioDecoder) empty meta data field.";
}


QStringList K3b::AudioDecoder::supportedTechnicalInfos() const
{
    QStringList l;
    for( QMap<QString, QString>::const_iterator it = d->technicalInfoMap.constBegin();
         it != d->technicalInfoMap.constEnd(); ++it )
        l.append( it.key() );
    return l;
}


QString K3b::AudioDecoder::technicalInfo( const QString& key ) const
{
    return d->technicalInfoMap[key];
}


void K3b::AudioDecoder::addTechnicalInfo( const QString& key, const QString& value )
{
    d->technicalInfoMap[key] = value;
}


K3b::AudioDecoder* K3b::AudioDecoderFactory::createDecoder( const KUrl& url )
{
    kDebug() << "(K3b::AudioDecoderFactory::createDecoder( " << url.path() << " )";
    QList<K3b::Plugin*> fl = k3bcore->pluginManager()->plugins( "AudioDecoder" );

    // first search for a single format decoder
    Q_FOREACH( K3b::Plugin* plugin, fl ) {
        K3b::AudioDecoderFactory* f = dynamic_cast<K3b::AudioDecoderFactory*>( plugin );
        if( f && !f->multiFormatDecoder() && f->canDecode( url ) ) {
            kDebug() << "1"; return f->createDecoder();}
    }

    // no single format decoder. Search for a multi format decoder
    Q_FOREACH( K3b::Plugin* plugin, fl ) {
        K3b::AudioDecoderFactory* f = dynamic_cast<K3b::AudioDecoderFactory*>( plugin );
        if( f && f->multiFormatDecoder() && f->canDecode( url ) ) {
            kDebug() << "2"; return f->createDecoder();}
    }

    kDebug() << "(K3b::AudioDecoderFactory::createDecoder( " << url.path() << " ) no success";

    // nothing found
    return 0;
}


QString K3b::AudioDecoderFactory::categoryName() const
{
    return i18nc( "plugin type", "Audio Decoder" );
}

#include "k3baudiodecoder.moc"
