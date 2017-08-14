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

#include "k3bdeviceglobals.h"
#include "k3bdevice.h"
#include "k3bdevice_i18n.h"
#include "k3bdiskinfo.h"

#include <QtCore/QDebug>
#include <QtCore/QStringList>


QString K3b::Device::deviceTypeString( int t )
{
    QStringList s;
    if( t & K3b::Device::DEVICE_CD_R )
        s += i18n("CD-R");
    if( t & K3b::Device::DEVICE_CD_RW )
        s += i18n("CD-RW");
    if( t & K3b::Device::DEVICE_CD_ROM )
        s += i18n("CD-ROM");
    if( t & K3b::Device::DEVICE_DVD_ROM )
        s += i18n("DVD-ROM");
    if( t & K3b::Device::DEVICE_DVD_RAM )
        s += i18n("DVD-RAM");
    if( t & K3b::Device::DEVICE_DVD_R )
        s += i18n("DVD-R");
    if( t & K3b::Device::DEVICE_DVD_RW )
        s += i18n("DVD-RW");
    if( t & K3b::Device::DEVICE_DVD_R_DL )
        s += i18n("DVD-R DL");
    if( t & DEVICE_HD_DVD_ROM )
        s += i18n("HD DVD-ROM");
    if( t & DEVICE_HD_DVD_R )
        s += i18n("HD DVD-R");
    if( t & DEVICE_HD_DVD_RAM )
        s += i18n("HD DVD-RAM");
    if( t & DEVICE_BD_ROM )
        s += i18n("BD-ROM");
    if( t & DEVICE_BD_R )
        s += i18n("BD-R");
    if( t & DEVICE_BD_RE )
        s += i18n("BD-RE");
    if( t & K3b::Device::DEVICE_DVD_PLUS_R )
        s += i18n("DVD+R");
    if( t & K3b::Device::DEVICE_DVD_PLUS_RW )
        s += i18n("DVD+RW");
    if( t & K3b::Device::DEVICE_DVD_PLUS_R_DL )
        s += i18n("DVD+R DL");

    if( s.isEmpty() )
        return i18n("Error");
    else
        return s.join( ", " );
}


QString K3b::Device::writingModeString( int m )
{
    QStringList s;
    if( m & K3b::Device::WRITINGMODE_SAO )
        s += i18n("SAO");
    if( m & K3b::Device::WRITINGMODE_TAO )
        s += i18n("TAO");
    if( m & K3b::Device::WRITINGMODE_RAW )
        s += i18n("RAW");
    if( m & K3b::Device::WRITINGMODE_SAO_R96P )
        s += i18n("SAO/R96P");
    if( m & K3b::Device::WRITINGMODE_SAO_R96R )
        s += i18n("SAO/R96R");
    if( m & K3b::Device::WRITINGMODE_RAW_R16 )
        s += i18n("RAW/R16");
    if( m & K3b::Device::WRITINGMODE_RAW_R96P )
        s += i18n("RAW/R96P");
    if( m & K3b::Device::WRITINGMODE_RAW_R96R )
        s += i18n("RAW/R96R");
    if( m & K3b::Device::WRITINGMODE_INCR_SEQ )
        s += i18n("Incremental Sequential");
    if( m & K3b::Device::WRITINGMODE_RES_OVWR )
        s += i18n("Restricted Overwrite");
    if( m & K3b::Device::WRITINGMODE_LAYER_JUMP )
        s += i18n("Layer Jump");

    if( m & K3b::Device::WRITINGMODE_RRM )
        s += i18n("Random Recording");
    if( m & K3b::Device::WRITINGMODE_SRM )
        s += i18n("Sequential Recording");
    if( m & K3b::Device::WRITINGMODE_SRM_POW )
        s += i18n("Sequential Recording + POW");

    if( s.isEmpty() )
        return i18nc("no writing mode", "None");
    else
        return s.join( ", " );
}


QString K3b::Device::mediaTypeString( int m, bool simple )
{
    if( m == K3b::Device::MEDIA_UNKNOWN )
        return i18nc("unknown medium type", "Unknown");

    QStringList s;
    if( m & MEDIA_NONE )
        s += i18n("No media");
    if( m & MEDIA_DVD_ROM )
        s += i18n("DVD-ROM");
    if( m & MEDIA_DVD_R ||
        (simple && (m & MEDIA_DVD_R_SEQ)) )
        s += i18n("DVD-R");
    if( m & MEDIA_DVD_R_SEQ && !simple )
        s += i18n("DVD-R Sequential");
    if( m & MEDIA_DVD_R_DL ||
        (simple && (m & (MEDIA_DVD_R_DL_SEQ|MEDIA_DVD_R_DL_JUMP))) )
        s += i18n("DVD-R Dual Layer");
    if( m & MEDIA_DVD_R_DL_SEQ && !simple )
        s += i18n("DVD-R Dual Layer Sequential");
    if( m & MEDIA_DVD_R_DL_JUMP && !simple )
        s += i18n("DVD-R Dual Layer Jump");
    if( m & MEDIA_DVD_RAM )
        s += i18n("DVD-RAM");
    if( m & MEDIA_DVD_RW ||
        (simple && (m & (MEDIA_DVD_RW_OVWR|MEDIA_DVD_RW_SEQ))) )
        s += i18n("DVD-RW");
    if( m & MEDIA_DVD_RW_OVWR && !simple )
        s += i18n("DVD-RW Restricted Overwrite");
    if( m & MEDIA_DVD_RW_SEQ && !simple )
        s += i18n("DVD-RW Sequential");
    if( m & MEDIA_DVD_PLUS_RW )
        s += i18n("DVD+RW");
    if( m & MEDIA_DVD_PLUS_R )
        s += i18n("DVD+R");
    if( m & MEDIA_DVD_PLUS_RW_DL )
        s += i18n("DVD+RW Dual Layer");
    if( m & MEDIA_DVD_PLUS_R_DL )
        s += i18n("DVD+R Dual Layer");
    if( m & MEDIA_CD_ROM )
        s += i18n("CD-ROM");
    if( m & MEDIA_CD_R )
        s += i18n("CD-R");
    if( m & MEDIA_CD_RW )
        s += i18n("CD-RW");
    if( m & MEDIA_HD_DVD_ROM )
        s += i18n("HD DVD-ROM");
    if( m & MEDIA_HD_DVD_R )
        s += i18n("HD DVD-R");
    if( m & MEDIA_HD_DVD_RAM )
        s += i18n("HD DVD-RAM");
    if( m & MEDIA_BD_ROM )
        s += i18n("BD-ROM");
    if( m & MEDIA_BD_R ||
        (simple && (m & (MEDIA_BD_R_SRM|MEDIA_BD_R_RRM))) )
        s += i18n("BD-R");
    if( m & MEDIA_BD_R_SRM && !simple )
        s += i18n("BD-R Sequential (SRM)");
    if( m & MEDIA_BD_R_SRM_POW && !simple )
        s += i18n("BD-R Sequential Pseudo Overwrite (SRM+POW)");
    if( m & MEDIA_BD_R_RRM && !simple )
        s += i18n("BD-R Random (RRM)");
    if( m & MEDIA_BD_RE )
        s += i18n("BD-RE");

    if( s.isEmpty() )
        return i18n("Error");
    else
        return s.join( ", " );
}


QString K3b::Device::mediaStateString( int state )
{
    if( state == K3b::Device::STATE_UNKNOWN )
        return i18nc("unknown medium state", "Unknown");

    QStringList s;
    if( state & STATE_NO_MEDIA )
        s += i18n("no medium");
    if( state & STATE_COMPLETE )
        s += i18n("complete medium");
    if( state & STATE_INCOMPLETE )
        s += i18n("incomplete medium");
    if( state & STATE_EMPTY )
        s += i18n("empty medium");

    if( s.isEmpty() )
        return i18n("Error");
    else
        return s.join( ", " );
}


void K3b::Device::debugBitfield( unsigned char* data, long len )
{
    for( int i = 0; i < len; ++i ) {
        QString index, bitString;
        index.sprintf( "%4i", i );
        for( int bp = 7; bp >= 0; --bp )
            bitString[7-bp] = ( data[i] & (1<<bp) ? '1' : '0' );
        qDebug() << index << " - " << bitString << " - " << (int)data[i];
    }
}


quint16 K3b::Device::from2Byte(const unsigned char* d)
{
    if (d == NULL) {
        qWarning() << "Invalid Byte!";
        return 0;
    }
    return ((d[0] << 8 & 0xFF00) |
            (d[1]      & 0xFF));
}


quint32 K3b::Device::from4Byte(const unsigned char* d)
{
    if (d == NULL) {
        qWarning() << "Invalid Byte!";
        return 0;
    }
    return ((d[0] << 24 & 0xFF000000) |
            (d[1] << 16 & 0xFF0000)   |
            (d[2] << 8  & 0xFF00)     |
            (d[3]       & 0xFF));
}


char K3b::Device::fromBcd( const char& i )
{
    return (i & 0x0f) + 10 * ( (i >> 4) & 0x0f );
}


char K3b::Device::toBcd( const char& i )
{
    return ( i % 10 ) | ( ( (( i / 10 ) % 10) << 4 ) & 0xf0 );
}


bool K3b::Device::isValidBcd( const char& i )
{
    return ( i & 0x0f ) <= 0x09 && ( i & 0xf0 ) <= 0x90;
}


int K3b::Device::determineMaxReadingBufferSize( K3b::Device::Device* dev, const K3b::Msf& firstSector )
{
    //
    // As long as we do not know how to determine the max read buffer properly we simply determine it
    // by trying. :)
    //

    int bufferSizeSectors = 128;
    unsigned char buffer[2048*128];
    while( !dev->read10( buffer, 2048*bufferSizeSectors, firstSector.lba(), bufferSizeSectors ) ) {
        qDebug() << "(K3b::DataTrackReader) determine max read sectors: "
                 << bufferSizeSectors << " too high." << endl;
        bufferSizeSectors--;
    }
    qDebug() << "(K3b::DataTrackReader) determine max read sectors: "
             << bufferSizeSectors << " is max." << endl;

    return bufferSizeSectors;
}


QDebug& K3b::Device::operator<<( QDebug& dbg, K3b::Device::MediaType type )
{
    return dbg << mediaTypeString( type );
}


QDebug& K3b::Device::operator<<( QDebug& dbg, K3b::Device::MediaTypes types )
{
    return dbg << mediaTypeString( types );
}


QDebug& K3b::Device::operator<<( QDebug& dbg, K3b::Device::WritingMode mode )
{
    return dbg << writingModeString( mode );
}


QDebug& K3b::Device::operator<<( QDebug& dbg, K3b::Device::WritingModes modes )
{
    return dbg << writingModeString( modes );
}


QDebug& K3b::Device::operator<<( QDebug& dbg, K3b::Device::MediaState state )
{
    return dbg << mediaStateString( state );
}


QDebug& K3b::Device::operator<<( QDebug& dbg, K3b::Device::MediaStates states )
{
    return dbg << mediaStateString( states );
}
