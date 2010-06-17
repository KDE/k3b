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

#include "k3blameencoder.h"
#include "k3blameencoderdefaults.h"
#include "k3blametyes.h"

#include <config-k3b.h>

#include "k3bcore.h"

#include <KLocale>
#include <KConfig>
#include <KDebug>
#include <QTextCodec>

#include <stdio.h>
#include <lame/lame.h>

#include <QFile>


class K3bLameEncoder::Private
{
public:
    Private()
        : flags(0),
          fid(0) {
    }

    lame_global_flags* flags;

    char buffer[8000];

    QString filename;
    FILE* fid;
};




K3bLameEncoder::K3bLameEncoder( QObject* parent, const QVariantList& )
    : K3b::AudioEncoder( parent )
{
    d = new Private();
}


K3bLameEncoder::~K3bLameEncoder()
{
    closeFile();

    delete d;
}


bool K3bLameEncoder::openFile( const QString& extension, const QString& filename, const K3b::Msf& length, const MetaData& metaData )
{
    closeFile();

    d->filename = filename;
    d->fid = ::fopen( QFile::encodeName( filename ), "w+" );
    if( d->fid )
        return initEncoder( extension, length, metaData );
    else
        return false;
}


bool K3bLameEncoder::isOpen() const
{
    return ( d->fid != 0 );
}


void K3bLameEncoder::closeFile()
{
    if( isOpen() ) {
        finishEncoder();
        ::fclose( d->fid );
        d->fid = 0;
        d->filename.truncate(0);
    }
}


QString K3bLameEncoder::filename() const
{
    return d->filename;
}


bool K3bLameEncoder::initEncoderInternal( const QString&, const K3b::Msf& length, const MetaData& metaData )
{
    KSharedConfig::Ptr c = KGlobal::config();
    KConfigGroup grp(c, "K3bLameEncoderPlugin" );

    d->flags = lame_init();

    if( d->flags == 0 ) {
        kDebug() << "(K3bLameEncoder) lame_init failed.";
        return false;
    }

    //
    // set the format of the input data
    //
    lame_set_num_samples( d->flags, length.lba()*588 );
    lame_set_in_samplerate( d->flags, 44100 );
    lame_set_num_channels( d->flags, 2 );

    //
    // Lame by default determines the samplerate based on the bitrate
    // since we have no option for the user to influence this yet
    // we just keep to the good old 44.1 khz
    //
    lame_set_out_samplerate( d->flags, 44100 );

    //
    // Choose the quality level
    //
    if( grp.readEntry( "Manual Bitrate Settings", DEFAULT_MANUAL_BITRATE ) ) {
        //
        // Mode
        //
        QString mode = grp.readEntry( "Mode", DEFAULT_MODE );
        if( mode == "stereo" )
            lame_set_mode( d->flags, STEREO );
        else if( mode == "joint" )
            lame_set_mode( d->flags, JOINT_STEREO );
        else // mono
            lame_set_mode( d->flags, MONO );

        //
        // Variable Bitrate
        //
        if( grp.readEntry( "VBR", DEFAULT_VBR ) ) {
            // we use the default algorithm here
            lame_set_VBR( d->flags, vbr_default );

            if( grp.readEntry( "Use Maximum Bitrate", DEFAULT_USE_MAXIMUM_BITRATE ) ) {
                lame_set_VBR_max_bitrate_kbps( d->flags, grp.readEntry( "Maximum Bitrate", DEFAULT_MAXIMUM_BITRATE ) );
            }
            if( grp.readEntry( "Use Minimum Bitrate", DEFAULT_USE_MINIMUM_BITRATE ) ) {
                lame_set_VBR_min_bitrate_kbps( d->flags, grp.readEntry( "Minimum Bitrate", DEFAULT_MINIMUM_BITRATE ) );

                // TODO: lame_set_hard_min
            }
            if( grp.readEntry( "Use Average Bitrate", DEFAULT_USE_AVERAGE_BITRATE ) ) {
                lame_set_VBR( d->flags, vbr_abr );
                lame_set_VBR_mean_bitrate_kbps( d->flags, grp.readEntry( "Average Bitrate", DEFAULT_AVERAGE_BITRATE ) );
            }
        }

        //
        // Constant Bitrate
        //
        else {
            lame_set_VBR( d->flags, vbr_off );
            lame_set_brate( d->flags, grp.readEntry( "Constant Bitrate", DEFAULT_CONSTANT_BITRATE ) );
        }
    }

    else {
        //
        // In lame 0 is the highest quality. Since that is just confusing for the user
        // if we call the setting "Quality" we simply invert the value.
        //
        int q = grp.readEntry( "Quality Level", DEFAULT_QUALITY_LEVEL );
        if( q < 0 ) q = 0;
        if( q > 9 ) q = 9;

        kDebug() << "(K3bLameEncoder) setting preset encoding value to " << q;

        if ( q < 2 || q > 8 ) {
            lame_set_VBR( d->flags, vbr_abr );
        }
        else {
            lame_set_VBR( d->flags, vbr_default );
        }
        lame_set_preset( d->flags, s_lame_presets[q] );

        if( q < 2 )
            lame_set_mode( d->flags, MONO );
    }


    //
    // file options
    //
    lame_set_copyright( d->flags, grp.readEntry( "Copyright", DEFAULT_COPYRIGHT ) );
    lame_set_original( d->flags, grp.readEntry( "Original", DEFAULT_ORIGINAL ) );
    lame_set_strict_ISO( d->flags, grp.readEntry( "ISO compliance", DEFAULT_ISO_COMPLIANCE ) );
    lame_set_error_protection( d->flags, grp.readEntry( "Error Protection", DEFAULT_ERROR_PROTECTION ) );


    //
    // Used Algorithm
    //
    // default to 2 which is the same as the -h lame option
    // THIS HAS NO INFLUENCE ON THE SIZE OF THE FILE!
    //
    //
    // In lame 0 is the highest quality. Since that is just confusing for the user
    // if we call the setting "Quality" we simply invert the value.
    //
    int q = grp.readEntry( "Encoder Quality", DEFAULT_ENCODER_QUALITY );
    if( q < 0 ) q = 0;
    if( q > 9 ) q = 9;
    lame_set_quality( d->flags, 9-q );

    //
    // ID3 settings
    //
    // for now we default to both v1 and v2 tags
    id3tag_add_v2( d->flags );
    id3tag_pad_v2( d->flags );

    // let's not use UTF-8 here since I don't know how to tell lame...
    // FIXME: when we use the codec we only get garbage. Why?
    QTextCodec* codec = 0;//QTextCodec::codecForName( "ISO8859-1" );
//  if( !codec )
//    kDebug() << "(K3bLameEncoder) could not find codec ISO8859-1.";

    for( MetaData::const_iterator it = metaData.constBegin(); it != metaData.constEnd(); ++it ) {
        QByteArray value = codec ? codec->fromUnicode( it.value().toString() ).data()
                                 : it.value().toString().toLatin1().data();
        switch( it.key() ) {
        case META_TRACK_TITLE:
            id3tag_set_title( d->flags, value );
            break;
        case META_TRACK_ARTIST:
            id3tag_set_artist( d->flags, value );
            break;
        case META_ALBUM_TITLE:
            id3tag_set_album( d->flags, value );
            break;
        case META_ALBUM_COMMENT:
            id3tag_set_comment( d->flags, value );
            break;
        case META_YEAR:
            id3tag_set_year( d->flags, value );
            break;
        case META_TRACK_NUMBER:
            id3tag_set_track( d->flags, value );
            break;
        case META_GENRE:
            if( id3tag_set_genre( d->flags, value ) )
                kDebug() << "(K3bLameEncoder) unable to set genre.";
            break;
        default:
            break;
        }
    }

    return( lame_init_params( d->flags ) != -1 );
}


qint64 K3bLameEncoder::encodeInternal( const char* data, qint64 len )
{
    // FIXME: we may have to swap data here
    int size = lame_encode_buffer_interleaved( d->flags,
                                               (short int*)data,
                                               len/4,
                                               (unsigned char*)d->buffer,
                                               8000 );
    if( size < 0 ) {
        kDebug() << "(K3bLameEncoder) lame_encode_buffer_interleaved failed.";
        return -1;
    }

    return ::fwrite( d->buffer, 1, size, d->fid );
}


void K3bLameEncoder::finishEncoderInternal()
{
    int size = lame_encode_flush( d->flags,
                                  (unsigned char*)d->buffer,
                                  8000 );
    if( size > 0 )
        ::fwrite( d->buffer, 1, size, d->fid );

    lame_mp3_tags_fid( d->flags, d->fid );

    lame_close( d->flags );
    d->flags = 0;
}


QStringList K3bLameEncoder::extensions() const
{
    return QStringList( "mp3" );
}


QString K3bLameEncoder::fileTypeComment( const QString& ) const
{
    return "MPEG1 Layer III (mp3)";
}


long long K3bLameEncoder::fileSize( const QString&, const K3b::Msf& msf ) const
{
    KSharedConfig::Ptr c = KGlobal::config();
    KConfigGroup grp(c, "K3bLameEncoderPlugin" );
    int bitrate = 0;
    if( grp.readEntry( "Manual Bitrate Settings", DEFAULT_MANUAL_BITRATE ) ) {
        if( grp.readEntry( "VBR", DEFAULT_VBR ) ) {
            if( grp.readEntry( "Use Maximum Bitrate", DEFAULT_USE_MAXIMUM_BITRATE ) )
                bitrate = grp.readEntry( "Maximum Bitrate", DEFAULT_MAXIMUM_BITRATE );
            if( grp.readEntry( "Use Minimum Bitrate", DEFAULT_USE_MINIMUM_BITRATE ) )
                bitrate = ( bitrate > 0
                            ? (bitrate - grp.readEntry( "Minimum Bitrate", DEFAULT_MINIMUM_BITRATE )) / 2
                            : grp.readEntry( "Minimum Bitrate", DEFAULT_MINIMUM_BITRATE ) );
            if( grp.readEntry( "Use Average Bitrate", DEFAULT_USE_AVERAGE_BITRATE ) )
                bitrate = grp.readEntry( "Average Bitrate", DEFAULT_AVERAGE_BITRATE );
        }
        else {
            bitrate = grp.readEntry( "Constant Bitrate", DEFAULT_CONSTANT_BITRATE );
        }
    }
    else {
        int q = grp.readEntry( "Quality Level", DEFAULT_ENCODER_QUALITY );
        if( q < 0 ) q = 0;
        if( q > 9 ) q = 9;
        bitrate = s_lame_preset_approx_bitrates[q];
    }

    return (msf.totalFrames()/75 * bitrate * 1000)/8;
}

#include "k3blameencoder.moc"
