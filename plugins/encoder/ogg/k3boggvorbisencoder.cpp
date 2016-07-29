/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include "k3boggvorbisencoder.h"
#include "k3boggvorbisencoderdefaults.h"
#include <config-k3b.h>

#include "k3bcore.h"

#include <KConfig>
#include <KDebug>
#include <KLocale>

#include <vorbis/vorbisenc.h>

// for the random generator
#include <stdlib.h>
#include <time.h>


// quality levels -1 to 10 map to 0 to 11
static const int s_rough_average_quality_level_bitrates[] = {
    45,
    64,
    80,
    96,
    112,
    128,
    160,
    192,
    224,
    256,
    320,
    400
};

// quality levels -1 to 10 map to 0 to 11
// static const char* s_ogg_quality_level_strings[] = {
//   I18N_NOOP("Low quality"),
//   I18N_NOOP(""),
//   I18N_NOOP(""),
//   I18N_NOOP(""),
//   I18N_NOOP(""),
//   I18N_NOOP("targetted %1 kbps"),
//   I18N_NOOP("targetted %1 kbps"),
//   I18N_NOOP("targetted %1 kbps"),
//   I18N_NOOP(""),
//   I18N_NOOP(""),
//   I18N_NOOP(""),
//   I18N_NOOP(""),
// };


// THIS IS BASED ON THE OGG VORBIS LIB EXAMPLE
// BECAUSE OF THE LACK OF DOCUMENTATION


class K3bOggVorbisEncoder::Private
{
public:
    Private()
        : manualBitrate(false),
          qualityLevel(4),
          bitrateUpper(-1),
          bitrateNominal(-1),
          bitrateLower(-1),
          //      sampleRate(44100),
          oggStream(0),
          oggPage(0),
          oggPacket(0),
          vorbisInfo(0),
          vorbisComment(0),
          vorbisDspState(0),
          vorbisBlock(0),
          headersWritten(false) {
    }

    // encoding settings
    bool manualBitrate;
    // 0 to 10 -> 0.0 - 1.0
    int qualityLevel;
    int bitrateUpper;
    int bitrateNominal;
    int bitrateLower;
    //  int sampleRate;

    // encoding structures
    ogg_stream_state *oggStream;       // take physical pages, weld into a logical stream of packets
    ogg_page         *oggPage;         // one Ogg bitstream page.  Vorbis packets are inside
    ogg_packet       *oggPacket;       // one raw packet of data for decode
    vorbis_info      *vorbisInfo;      // struct that stores all the static vorbis bitstream settings
    vorbis_comment   *vorbisComment;   // struct that stores all the user comments
    vorbis_dsp_state *vorbisDspState;  // central working state for the packet->PCM decoder
    vorbis_block     *vorbisBlock;     // local working space for packet->PCM decode

    bool headersWritten;
};


K3bOggVorbisEncoder::K3bOggVorbisEncoder( QObject* parent,  const QVariantList& )
    : K3b::AudioEncoder( parent )
{
    d = new Private();
}


K3bOggVorbisEncoder::~K3bOggVorbisEncoder()
{
    cleanup();
    delete d;
}


bool K3bOggVorbisEncoder::initEncoderInternal( const QString&, const K3b::Msf& /*length*/, const MetaData& metaData )
{
    cleanup();

    // load user settings
    loadConfig();

    d->oggPage = new ogg_page;
    d->oggPacket = new ogg_packet;
    d->vorbisInfo = new vorbis_info;

    vorbis_info_init( d->vorbisInfo );

    int ret = 0;

    if( d->manualBitrate ) {
        kDebug() << "(K3bOggVorbisEncoder) calling: "
                 << "vorbis_encode_init( d->vorbisInfo, 2, 44100, "
                 << (d->bitrateUpper != -1 ? d->bitrateUpper*1000 : -1) << ", "
                 << (d->bitrateNominal != -1 ? d->bitrateNominal*1000 : -1)  << ", "
                 << (d->bitrateLower != -1 ? d->bitrateLower*1000 : -1) << " );" << endl;

        ret = vorbis_encode_init( d->vorbisInfo,
                                  2, // 2 channels: stereo
                                  44100,
                                  d->bitrateUpper != -1 ? d->bitrateUpper*1000 : -1,
                                  d->bitrateNominal != -1 ? d->bitrateNominal*1000 : -1,
                                  d->bitrateLower != -1 ? d->bitrateLower*1000 : -1 );
    }
    else {
        if( d->qualityLevel < -1 )
            d->qualityLevel = -1;
        else if( d->qualityLevel > 10 )
            d->qualityLevel = 10;

        kDebug() << "(K3bOggVorbisEncoder) calling: "
                 << "vorbis_encode_init_vbr( d->vorbisInfo, 2, 44100, "
                 << (float)d->qualityLevel/10.0 << ");" << endl;

        ret = vorbis_encode_init_vbr( d->vorbisInfo,
                                      2, // 2 channels: stereo
                                      44100,
                                      (float)d->qualityLevel/10.0 );
    }

    if( ret ) {
        kDebug() << "(K3bOggVorbisEncoder) vorbis_encode_init failed: " << ret;
        cleanup();
        return false;
    }

    // init the comment stuff
    d->vorbisComment = new vorbis_comment;
    vorbis_comment_init( d->vorbisComment );

    // add the encoder tag (so everybody knows we did it! ;)
    vorbis_comment_add_tag( d->vorbisComment, QByteArray("ENCODER").data(), QByteArray("K3bOggVorbisEncoderPlugin").data() );

    // set up the analysis state and auxiliary encoding storage
    d->vorbisDspState = new vorbis_dsp_state;
    d->vorbisBlock = new vorbis_block;
    vorbis_analysis_init( d->vorbisDspState, d->vorbisInfo );
    vorbis_block_init( d->vorbisDspState, d->vorbisBlock );

    // set up our packet->stream encoder
    // pick a random serial number; that way we can more likely build
    // chained streams just by concatenation
    d->oggStream = new ogg_stream_state;
    srand( time(0) );
    ogg_stream_init( d->oggStream, rand() );

    // Set meta data
    for( MetaData::const_iterator it = metaData.constBegin(); it != metaData.constEnd(); ++it ) {
        QByteArray key;

        switch( it.key() ) {
        case META_TRACK_TITLE:
            key = "TITLE";
            break;
        case META_TRACK_ARTIST:
            key = "ARTIST";
            break;
        case META_ALBUM_TITLE:
            key = "ALBUM";
            break;
        case META_ALBUM_COMMENT:
            key = "DESCRIPTION";
            break;
        case META_YEAR:
            key = "DATE";
            break;
        case META_TRACK_NUMBER:
            key = "TRACKNUMBER";
            break;
        case META_GENRE:
            key = "GENRE";
            break;
        default:
            break;
        }

        if( !key.isEmpty() ) {
            vorbis_comment_add_tag( d->vorbisComment, key.data(), it.value().toString().toUtf8().data() );
        }
    }

    return true;
}


bool K3bOggVorbisEncoder::writeOggHeaders()
{
    if( !d->oggStream ) {
        kDebug() << "(K3bOggVorbisEncoder) call to writeOggHeaders without init.";
        return false;
    }
    if( d->headersWritten ) {
        kDebug() << "(K3bOggVorbisEncoder) headers already written.";
        return true;
    }

    //
    // Vorbis streams begin with three headers; the initial header (with
    // most of the codec setup parameters) which is mandated by the Ogg
    // bitstream spec.  The second header holds any comment fields.  The
    // third header holds the bitstream codebook.  We merely need to
    // make the headers, then pass them to libvorbis one at a time;
    // libvorbis handles the additional Ogg bitstream constraints
    //
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_headerout( d->vorbisDspState,
                               d->vorbisComment,
                               &header,
                               &header_comm,
                               &header_code);

    // automatically placed in its own page
    ogg_stream_packetin( d->oggStream, &header );
    ogg_stream_packetin( d->oggStream, &header_comm );
    ogg_stream_packetin( d->oggStream, &header_code );

    //
    // This ensures the actual
    // audio data will start on a new page, as per spec
    //
    QByteArray data;
    while( ogg_stream_flush( d->oggStream, d->oggPage ) ) {
        writeData( (char*)d->oggPage->header, d->oggPage->header_len );
        writeData( (char*)d->oggPage->body, d->oggPage->body_len );
    }

    d->headersWritten = true;

    return true;
}


qint64 K3bOggVorbisEncoder::encodeInternal( const char* data, qint64 len )
{
    if( !d->headersWritten )
        if( !writeOggHeaders() )
            return -1;

    // expose the buffer to submit data
    float** buffer = vorbis_analysis_buffer( d->vorbisDspState, len/4 );

    // uninterleave samples
    qint64 i = 0;
    for( i = 0; i < len/4; i++ ) {
        buffer[0][i]=( (data[i*4+1]<<8) | (0x00ff&(int)data[i*4]) ) / 32768.f;
        buffer[1][i]=( (data[i*4+3]<<8) | (0x00ff&(int)data[i*4+2]) ) / 32768.f;
    }

    // tell the library how much we actually submitted
    vorbis_analysis_wrote( d->vorbisDspState, i );

    return flushVorbis();
}


long K3bOggVorbisEncoder::flushVorbis()
{
    // vorbis does some data preanalysis, then divvies up blocks for
    // more involved (potentially parallel) processing.  Get a single
    // block for encoding now
    long writtenData = 0;
    while( vorbis_analysis_blockout( d->vorbisDspState, d->vorbisBlock ) == 1 ) {

        // analysis
        vorbis_analysis( d->vorbisBlock, 0 );
        vorbis_bitrate_addblock( d->vorbisBlock );

        while( vorbis_bitrate_flushpacket( d->vorbisDspState, d->oggPacket ) ) {

            // weld the packet into the bitstream
            ogg_stream_packetin( d->oggStream, d->oggPacket );

            // write out pages (if any)
            while( ogg_stream_pageout( d->oggStream, d->oggPage ) ) {
                writeData( (char*)d->oggPage->header, d->oggPage->header_len );
                writeData( (char*)d->oggPage->body, d->oggPage->body_len );

                writtenData += ( d->oggPage->header_len + d->oggPage->body_len );
            }
        }
    }

    return writtenData;
}


void K3bOggVorbisEncoder::finishEncoderInternal()
{
    if( d->vorbisDspState ) {
        vorbis_analysis_wrote( d->vorbisDspState, 0 );
        flushVorbis();
    }
    else
        kDebug() << "(K3bOggVorbisEncoder) call to finishEncoderInternal without init.";
}


void K3bOggVorbisEncoder::cleanup()
{
    if( d->oggStream ) {
        ogg_stream_clear( d->oggStream );
        delete d->oggStream;
        d->oggStream = 0;
    }
    if( d->vorbisBlock ) {
        vorbis_block_clear( d->vorbisBlock );
        delete d->vorbisBlock;
        d->vorbisBlock = 0;
    }
    if( d->vorbisDspState ) {
        vorbis_dsp_clear( d->vorbisDspState );
        delete d->vorbisDspState;
        d->vorbisDspState = 0;
    }
    if( d->vorbisComment ) {
        vorbis_comment_clear( d->vorbisComment );
        delete d->vorbisComment;
        d->vorbisComment = 0;
    }
    if( d->vorbisInfo ) {
        vorbis_info_clear( d->vorbisInfo );
        delete d->vorbisInfo;
        d->vorbisInfo = 0;
    }

    // ogg_page and ogg_packet structs always point to storage in
    // libvorbis.  They're never freed or manipulated directly
    if( d->oggPage ) {
        delete d->oggPage;
        d->oggPage = 0;
    }
    if( d->oggPacket ) {
        delete d->oggPacket;
        d->oggPacket = 0;
    }

    d->headersWritten = false;
}


void K3bOggVorbisEncoder::loadConfig()
{
    KSharedConfig::Ptr c = KGlobal::config();
    KConfigGroup grp(c, "K3bOggVorbisEncoderPlugin" );

    d->manualBitrate = grp.readEntry( "manual bitrate", DEFAULT_MANUAL_BITRATE );
    d->qualityLevel = grp.readEntry( "quality level", DEFAULT_QUALITY_LEVEL );
    d->bitrateUpper = grp.readEntry( "bitrate upper", DEFAULT_BITRATE_UPPER );
    d->bitrateNominal = grp.readEntry( "bitrate nominal", DEFAULT_BITRATE_NOMINAL );
    d->bitrateLower = grp.readEntry( "bitrate lower", DEFAULT_BITRATE_LOWER );
    //  d->sampleRate = c->readEntry( "samplerate", DEFAULT_SAMPLERATE );
}


QString K3bOggVorbisEncoder::fileTypeComment( const QString& ) const
{
    return i18n("Ogg-Vorbis");
}


long long K3bOggVorbisEncoder::fileSize( const QString&, const K3b::Msf& msf ) const
{
    KSharedConfig::Ptr c = KGlobal::config();
    KConfigGroup grp(c, "K3bOggVorbisEncoderPlugin" );

    // the following code is based on the size estimation from the audiocd kioslave
    // TODO: reimplement.

    if( !grp.readEntry( "manual bitrate", DEFAULT_MANUAL_BITRATE ) ) {
        // Estimated numbers based on the Vorbis FAQ:
        // http://www.xiph.org/archives/vorbis-faq/200203/0030.html

//     static long vorbis_q_bitrate[] = { 45, 60,  74,  86,  106, 120, 152,
// 				       183, 207, 239, 309, 440 };

        int qualityLevel = grp.readEntry( "quality level", DEFAULT_QUALITY_LEVEL );

        if( qualityLevel < -1 )
            qualityLevel = -1;
        else if( qualityLevel > 10 )
            qualityLevel = 10;
        return ( (msf.totalFrames()/75) * s_rough_average_quality_level_bitrates[qualityLevel+1] * 1000 ) / 8;
    }
    else {
        return (msf.totalFrames()/75) * grp.readEntry( "bitrate nominal", 160 ) * 1000 / 8;
    }
}

#include "k3boggvorbisencoder.moc"
