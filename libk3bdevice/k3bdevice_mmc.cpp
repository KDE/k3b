/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

/**
   This file contains all the MMC command implementations of the K3b device class
   to make the code more readable.
**/


#include "k3bdevice.h"
#include "k3bscsicommand.h"
#include "k3bdeviceglobals.h"
#include "QDebug"

#include <string.h>


bool K3b::Device::Device::testUnitReady() const
{
    ScsiCommand cmd( this );
    cmd.enableErrorMessages( false );
    cmd[0] = MMC_TEST_UNIT_READY;
    cmd[5] = 0; // Necessary to set the proper command length
    return( cmd.transport() == 0 );
}


bool K3b::Device::Device::getFeature( UByteArray& data, unsigned int feature ) const
{
    unsigned char header[2048];
    ::memset( header, 0, 2048 );

    ScsiCommand cmd( this );
    cmd[0] = MMC_GET_CONFIGURATION;
    cmd[1] = 2;      // read only specified feature
    cmd[2] = feature>>8;
    cmd[3] = feature;
    cmd[8] = 8;      // we only read the data length first
    cmd[9] = 0;      // Necessary to set the proper command length

    // we only read the data length first
    unsigned int dataLen = 8;
    if( !cmd.transport( TR_DIR_READ, header, 8 ) )
        dataLen = from4Byte( header ) + 4;
    else
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": GET CONFIGURATION length det failed.";

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data or something invalid altogether.
    // So we simply use the maximum possible value to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( (dataLen-8) % 8 || dataLen <= 8 )
        dataLen = 0xFFFF;

    // again with real length
    data.resize( dataLen );
    ::memset( data.data(), 0, data.size() );

    cmd[7] = data.size() >> 8;
    cmd[8] = data.size();
    if( cmd.transport( TR_DIR_READ, data.data(), data.size() ) == 0 ) {
        data.resize( qMin( data.size(), (int)from4Byte( data.data() ) + 4 ) );
        return true;
    }
    else {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": GET CONFIGURATION with real length "
                 << data.size() << " failed." << endl;
        data.clear();
        return false;
    }
}


int K3b::Device::Device::featureCurrent( unsigned int feature ) const
{
    UByteArray data;
    if( getFeature( data, feature ) ) {
        int ret = -1;
        if( data.size() >= 11 )
            ret = ( data[8+2]&1 ? 1 : 0 );  // check the current flag

        return ret;
    }
    else
        return -1;
}


bool K3b::Device::Device::readIsrc( unsigned int track, QByteArray& isrc ) const
{
    UByteArray data;

    if( readSubChannel( data, 0x3, track ) ) {
        bool isrcValid = false;

        if( data.size() >= 8+18 ) {
            isrcValid = (data[8+4]>>7 & 0x1);

            if( isrcValid ) {
                isrc = QByteArray( reinterpret_cast<char*>(data[8+5]), 13 );

                // TODO: check the range of the chars

            }
        }

        return isrcValid;
    }
    else
        return false;
}


bool K3b::Device::Device::readMcn( QByteArray& mcn ) const
{
    UByteArray data;

    if( readSubChannel( data, 0x2, 0 ) ) {
        bool mcnValid = false;

        if( data.size() >= 8+18 ) {
            mcnValid = (data[8+4]>>7 & 0x1);

            if( mcnValid )
                mcn = QByteArray( reinterpret_cast<char*>(data[8+5]), 14 );
        }

        return mcnValid;
    }
    else
        return false;
}


bool K3b::Device::Device::getPerformance( UByteArray& data,
                                          unsigned int type,
                                          unsigned int dataType,
                                          unsigned int lba ) const
{
    unsigned int descLen = 0;
    switch( type ) {
    case 0x0:
        descLen = 16;
        break;
    case 0x1:
        descLen = 8;
        break;
    case 0x2:
        descLen = 2048;
        break;
    case 0x3:
        descLen = 16;
        break;
    case 0x4:
        descLen = 8;
        break;
    case 0x5:
        descLen = 8; // FIXME: ??
        break;
    }

    unsigned int dataLen = descLen + 8;
    UByteArray header( dataLen );
    ::memset( header.data(), 0, dataLen );

    ScsiCommand cmd( this );
    cmd[0] = MMC_GET_PERFORMANCE;
    cmd[1] = dataType;
    cmd[2] = lba >> 24;
    cmd[3] = lba >> 16;
    cmd[4] = lba >> 8;
    cmd[5] = lba;
    cmd[9] = 1;      // first we read one descriptor
    cmd[10] = type;
    cmd[11] = 0;     // Necessary to set the proper command length
    if( cmd.transport( TR_DIR_READ, header.data(), dataLen ) ) {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName()
                 << ": GET PERFORMANCE length det failed." << endl;
        return false;
    }

    dataLen = from4Byte( header.data() ) + 4;

    // At least one Panasonic drive returns gigantic changing numbers for the data length
    // which makes K3b crash below when *data cannot be allocated. That's why we cut the
    // length here.
    // FIXME: 2048 is a proper upper boundary for the write speed but not for all
    //        return types. "Defect Status Data" for example might return way more data.
    // FIXME: Since we only use getPerformance for writing speeds and without a proper length
    //        those do not make sense it is better to fail here anyway.
    if( descLen == 0 || (dataLen-8) % descLen || dataLen <= 8 || dataLen > 2048 ) {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName()
                 << ": GET PERFORMANCE reports bogus dataLen: " << dataLen << endl;
        return false;
    }

    data.resize( dataLen );
    ::memset( data.data(), 0, data.size() );

    unsigned int numDesc = (dataLen-8)/descLen;

    cmd[8] = numDesc>>8;
    cmd[9] = numDesc;
    if( cmd.transport( TR_DIR_READ, data.data(), data.size() ) == 0 ) {
        data.resize( qMin( data.size(), (int)from4Byte( data.data() ) + 4 ) );

        if( data.size() > 8 ) {
            return true;
        }
        else {
            qDebug() << "(K3b::Device::Device) " << blockDeviceName()
                    << ": GET PERFORMANCE reports invalid data size:" << data.size() << endl;
            data.clear();
            return false;
        }
    }
    else {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName()
                 << ": GET PERFORMANCE with real length "
                 << data.size() << " failed." << endl;
        data.clear();
        return false;
    }
}


bool K3b::Device::Device::setSpeed( unsigned int readingSpeed,
                                  unsigned int writingSpeed,
                                  bool cav ) const
{
    ScsiCommand cmd( this );
    cmd[0] = MMC_SET_SPEED;
    cmd[1] = ( cav ? 0x1 : 0x0 );
    cmd[2] = readingSpeed >> 8;
    cmd[3] = readingSpeed;
    cmd[4] = writingSpeed >> 8;
    cmd[5] = writingSpeed;
    cmd[11] = 0;      // Necessary to set the proper command length
    return ( cmd.transport( TR_DIR_WRITE ) == 0 );
}


bool K3b::Device::Device::seek( unsigned long lba ) const
{
    ScsiCommand cmd( this );
    cmd[0] = MMC_SEEK_10;
    cmd[2] = lba>>24;
    cmd[3] = lba>>16;
    cmd[4] = lba>>8;
    cmd[5] = lba;
    cmd[9] = 0;      // Necessary to set the proper command length
    return !cmd.transport();
}


bool K3b::Device::Device::readTrackInformation( UByteArray& data, int type, int value ) const
{
    unsigned char header[2048];
    ::memset( header, 0, 2048 );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_TRACK_INFORMATION;
    cmd[9] = 0;      // Necessary to set the proper command length

    switch( type ) {
    case 0:
    case 1:
    case 2:
        cmd[1] = type & 0x3;
        cmd[2] = value>>24;
        cmd[3] = value>>16;
        cmd[4] = value>>8;
        cmd[5] = value;
        break;
    default:
        qDebug() << "(K3b::Device::readTrackInformation) wrong type parameter: " << type;
        return false;
    }

    // first we read the header
    unsigned int dataLen = 4;
    cmd[8] = 4;
    if( cmd.transport( TR_DIR_READ, header, 4 ) == 0 )
        dataLen = from2Byte( header ) + 2;
    else
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ TRACK INFORMATION length det failed.";

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data.
    // So we try to determine the correct size based on the medium type
    // DVD+R:  40 (MMC4)
    // DVD-DL: 48 (MMC5)
    // CD:     36 (MMC2)
    //
    if( dataLen <= 6 ) {
        int m = mediaType();
        if( m & (MEDIA_DVD_R_DL|MEDIA_DVD_R_DL_SEQ|MEDIA_DVD_R_DL_JUMP) )
            dataLen = 48;
        else if( m & (MEDIA_DVD_PLUS_R|MEDIA_DVD_PLUS_R_DL) )
            dataLen = 40;
        else
            dataLen = 36;
    }

    // again with real length
    data.resize( dataLen );
    ::memset( data.data(), 0, data.size() );

    cmd[7] = data.size() >> 8;
    cmd[8] = data.size();
    if( cmd.transport( TR_DIR_READ, data.data(), data.size() ) == 0 ) {
        data.resize( qMin( data.size(), from2Byte( data.data() ) + 2 ) );
        return true;
    }
    else {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ TRACK INFORMATION with real length "
                 << data.size() << " failed." << endl;
        data.clear();
        return false;
    }
}



bool K3b::Device::Device::read10( unsigned char* data,
                                unsigned int dataLen,
                                unsigned long startAdress,
                                unsigned int length,
                                bool fua ) const
{
    ::memset( data, 0, dataLen );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_10;
    cmd[1] = ( fua ? 0x8 : 0x0 );
    cmd[2] = startAdress>>24;
    cmd[3] = startAdress>>16;
    cmd[4] = startAdress>>8;
    cmd[5] = startAdress;
    cmd[7] = length>>8;
    cmd[8] = length;
    cmd[9] = 0;      // Necessary to set the proper command length

    if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ 10 failed!";
        return false;
    }
    else
        return true;
}


bool K3b::Device::Device::read12( unsigned char* data,
                                unsigned int dataLen,
                                unsigned long startAdress,
                                unsigned long length,
                                bool streaming,
                                bool fua ) const
{
    ::memset( data, 0, dataLen );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_12;
    cmd[1] = ( fua ? 0x8 : 0x0 );
    cmd[2] = startAdress>>24;
    cmd[3] = startAdress>>16;
    cmd[4] = startAdress>>8;
    cmd[5] = startAdress;
    cmd[6] = length>>24;
    cmd[7] = length>>16;
    cmd[8] = length>>8;
    cmd[9] = length;
    cmd[10] = (streaming ? 0x80 : 0 );
    cmd[11] = 0;      // Necessary to set the proper command length

    if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ 12 failed!";
        return false;
    }
    else
        return true;
}


bool K3b::Device::Device::readCd( unsigned char* data,
                                unsigned int dataLen,
                                int sectorType,
                                bool dap,
                                unsigned long startAdress,
                                unsigned long length,
                                bool sync,
                                bool header,
                                bool subHeader,
                                bool userData,
                                bool edcEcc,
                                int c2,
                                int subChannel ) const
{
    ::memset( data, 0, dataLen );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_CD;
    cmd[1] = (sectorType<<2 & 0x1c) | ( dap ? 0x2 : 0x0 );
    cmd[2] = startAdress>>24;
    cmd[3] = startAdress>>16;
    cmd[4] = startAdress>>8;
    cmd[5] = startAdress;
    cmd[6] = length>>16;
    cmd[7] = length>>8;
    cmd[8] = length;
    cmd[9] = ( ( sync      ? 0x80 : 0x0 ) |
               ( subHeader ? 0x40 : 0x0 ) |
               ( header    ? 0x20 : 0x0 ) |
               ( userData  ? 0x10 : 0x0 ) |
               ( edcEcc    ? 0x8  : 0x0 ) |
               ( c2<<1 & 0x6 ) );
    cmd[10] = subChannel & 0x7;
    cmd[11] = 0;      // Necessary to set the proper command length

    if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ CD failed!";
        return false;
    }
    else {
        return true;
    }
}


bool K3b::Device::Device::readCdMsf( unsigned char* data,
                                   unsigned int dataLen,
                                   int sectorType,
                                   bool dap,
                                   const K3b::Msf& startAdress,
                                   const K3b::Msf& endAdress,
                                   bool sync,
                                   bool header,
                                   bool subHeader,
                                   bool userData,
                                   bool edcEcc,
                                   int c2,
                                   int subChannel ) const
{
    ::memset( data, 0, dataLen );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_CD_MSF;
    cmd[1] = (sectorType<<2 & 0x1c) | ( dap ? 0x2 : 0x0 );
    cmd[3] = (startAdress+150).minutes();
    cmd[4] = (startAdress+150).seconds();
    cmd[5] = (startAdress+150).frames();
    cmd[6] = (endAdress+150).minutes();
    cmd[7] = (endAdress+150).seconds();
    cmd[8] = (endAdress+150).frames();
    cmd[9] = ( ( sync      ? 0x80 : 0x0 ) |
               ( subHeader ? 0x40 : 0x0 ) |
               ( header    ? 0x20 : 0x0 ) |
               ( userData  ? 0x10 : 0x0 ) |
               ( edcEcc    ? 0x8  : 0x0 ) |
               ( c2<<1 & 0x6 ) );
    cmd[10] = subChannel & 0x7;
    cmd[11] = 0;      // Necessary to set the proper command length

    if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ CD MSF failed!";
        return false;
    }
    else
        return true;
}


bool K3b::Device::Device::readSubChannel( UByteArray& data,
                                        unsigned int subchannelParam,
                                        unsigned int trackNumber ) const
{
    unsigned char header[2048];
    ::memset( header, 0, 2048 );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_SUB_CHANNEL;
    cmd[2] = 0x40;    // SUBQ
    cmd[3] = subchannelParam;
    cmd[6] = trackNumber;   // only used when subchannelParam == 03h (ISRC)
    cmd[8] = 4;
    cmd[9] = 0;      // Necessary to set the proper command length

    // first we read the header
    unsigned int dataLen = 4;
    if( cmd.transport( TR_DIR_READ, header, 4 ) == 0 )
        dataLen = from2Byte( &header[2] ) + 4;
    else
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ SUB-CHANNEL length det failed.";

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we simply use the maximum possible value to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( dataLen <= 4 )
        dataLen = 0xFFFF;

    // again with real length
    data.resize( dataLen );
    ::memset( data.data(), 0, data.size() );

    cmd[7] = data.size() >> 8;
    cmd[8] = data.size();
    if( cmd.transport( TR_DIR_READ, data.data(), data.size() ) == 0 ) {
        data.resize( qMin( data.size(), from2Byte( &data[2] ) + 4 ) );
        return true;
    }
    else {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ SUB-CHANNEL with real length "
                 << data.size() << " failed." << endl;
        data.clear();
        return false;
    }
}


bool K3b::Device::Device::readTocPmaAtip( UByteArray& data, int format, bool time, int track ) const
{
    unsigned int descLen = 0;

    switch( format ) {
    case 0x0:
        descLen = 8;
        break;
    case 0x1:
        descLen = 8;
        break;
    case 0x2:
        descLen = 11;
        break;
    case 0x3:
        descLen = 11;
        break;
    case 0x4:
        descLen = 4; // MMC2: 24 and MMC4: 28, so we use the highest common factor
        break;
    case 0x5:
        descLen = 18;
        break;
    }

    unsigned char header[2048];
    ::memset( header, 0, 2048 );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_TOC_PMA_ATIP;
    cmd[1] = ( time ? 0x2 : 0x0 );
    cmd[2] = format & 0x0F;
    cmd[6] = track;
    cmd[8] = 4;
    cmd[9] = 0;      // Necessary to set the proper command length

    // we only read the header
    unsigned int dataLen = 4;
    if( cmd.transport( TR_DIR_READ, header, 4 ) == 0 )
        dataLen = from2Byte( header ) + 2;
    else
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ TOC/PMA/ATIP length det failed.";

    //
    // Some buggy firmwares return an invalid size here
    // So we simply use the maximum possible value to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( descLen != 0 && ((dataLen-4) % descLen || dataLen < 4+descLen) ) {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ TOC/PMA/ATIP invalid length returned: " << dataLen;
        dataLen = 0xFFFF;
    }

    //
    // Not all drives like uneven numbers
    //
    if( dataLen%2 )
        ++dataLen;

    // again with real length
    data.resize( dataLen );
    ::memset( data.data(), 0, data.size() );

    cmd[7] = data.size() >> 8;
    cmd[8] = data.size();
    if( cmd.transport( TR_DIR_READ, data.data(), data.size() ) == 0 ) {
        dataLen = qMin( dataLen, from2Byte( data.data() ) + 2u );
        if( descLen == 0 || (dataLen-4) % descLen || dataLen < 4+descLen ) {
            // useless length
            data.clear();
            return false;
        }
        else {
            data.resize( dataLen );
            return true;
        }
    }
    else {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ TOC/PMA/ATIP format "
                 << format << " with real length "
                 << data.size() << " failed." << endl;
        data.clear();
        return false;
    }
}


bool K3b::Device::Device::mechanismStatus( UByteArray& data ) const
{
    unsigned char header[2048];
    ::memset( header, 0, 2048 );

    ScsiCommand cmd( this );
    cmd[0] = MMC_MECHANISM_STATUS;
    cmd[9] = 8;
    cmd[11] = 0;    // Necessary to set the proper command length

    // first we read the header
    unsigned int dataLen = 8;
    if( cmd.transport( TR_DIR_READ, header, 8 ) == 0 )
        dataLen = from4Byte( &header[6] ) + 8;
    else
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": MECHANISM STATUS length det failed.";

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data or something invalid altogether.
    // So we simply use the maximum possible value to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( (dataLen-8) % 4 || dataLen <= 8 )
        dataLen = 0xFFFF;

    qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": MECHANISM STATUS "
             << (int)header[5] << " slots." << endl;

    // again with real length
    data.resize( dataLen );
    ::memset( data.data(), 0, data.size() );

    cmd[8] = data.size() >> 8;
    cmd[9] = data.size();
    if( cmd.transport( TR_DIR_READ, data.data(), data.size() ) == 0 ) {
        data.resize( qMin( data.size(), (int)from4Byte( &data[6] ) + 8 ) );
        return true;
    }
    else {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": MECHANISM STATUS with real length "
                 << data.size() << " failed." << endl;
        data.clear();
        return false;
    }
}



bool K3b::Device::Device::modeSense(UByteArray& pageData, int page) const
{
    unsigned char header[2048];
    ::memset(header, 0, sizeof(header));

    ScsiCommand cmd(this);
    cmd[0] = MMC_MODE_SENSE;
    cmd[1] = 0x8;         // Disable Block Descriptors
    cmd[2] = page & 0x3F;
    cmd[8] = 8;
    cmd[9] = 0;           // Necessary to set the proper command length

    // first we determine the data length
    int replyLen = 8;
    if (cmd.transport(TR_DIR_READ, header, 8) == 0)
        replyLen = from2Byte(header) + 2;
    else {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() <<
            ": MODE SENSE length det failed.";
    }
    // FIXME: rumor or misnomer?
    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we simply use the maximum possible value to be
    // on the safe side with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high
    // data length.
    if (replyLen == 8)
        replyLen = 0xFFFF;

    // again with real length
    pageData.resize(replyLen);
    ::memset(pageData.data(), 0, pageData.size());

    cmd[7] = pageData.size() >> 8;
    cmd[8] = pageData.size();
    if (cmd.transport(TR_DIR_READ, pageData.data(), pageData.size()) == 0 ) {
        pageData.resize(qMin(pageData.size(), from2Byte(pageData.data()) + 2));
        return true;
    } else {
        pageData.clear();
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() <<
            ": MODE SENSE with real length " << replyLen << " failed." << endl;
        return false;
    }
}


bool K3b::Device::Device::modeSelect( UByteArray& pageData, bool pf, bool sp ) const
{
    pageData[0] = 0;
    pageData[1] = 0;
    pageData[4] = 0;
    pageData[5] = 0;

    // we do not support Block Descriptors here
    pageData[6] = 0;
    pageData[7] = 0;

    // PS bit reserved
    pageData[8] &= 0x3F;

    ScsiCommand cmd( this );
    cmd[0] = MMC_MODE_SELECT;
    cmd[1] = ( sp ? 1 : 0 ) | ( pf ? 0x10 : 0 );
    cmd[7] = pageData.size()>>8;
    cmd[8] = pageData.size();
    cmd[9] = 0;
    return( cmd.transport( TR_DIR_WRITE, pageData.data(), pageData.size() ) == 0 );
}


// does only make sense for complete media
bool K3b::Device::Device::readCapacity( K3b::Msf& r ) const
{
    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_CAPACITY;
    cmd[9] = 0;      // Necessary to set the proper command length
    unsigned char buf[8];
    ::memset( buf, 0, 8 );
    if( cmd.transport( TR_DIR_READ, buf, 8 ) == 0 ) {
        r = from4Byte( buf );
        return true;
    }
    else
        return false;
}


bool K3b::Device::Device::readFormatCapacity( int wantedFormat, K3b::Msf& r,
                                              K3b::Msf* currentMax, int* currentMaxFormat ) const
{
    bool success = false;

    // the maximal length as stated in MMC4
    static const unsigned int maxLen = 4 + (8*32);

    unsigned char buffer[maxLen];
    ::memset( buffer, 0, maxLen );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_FORMAT_CAPACITIES;
    cmd[7] = maxLen >> 8;
    cmd[8] = maxLen & 0xFF;
    cmd[9] = 0;      // Necessary to set the proper command length
    if( cmd.transport( TR_DIR_READ, buffer, maxLen ) == 0 ) {

        unsigned int realLength = buffer[3] + 4;

        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << " READ FORMAT CAPACITY: Current/Max "
                 << (int)(buffer[8]&0x3) << " " << from4Byte( &buffer[4] ) << endl;

        if( currentMax )
            *currentMax = from4Byte( &buffer[4] );
        if( currentMaxFormat )
            *currentMaxFormat = (int)(buffer[8]&0x3);

        //
        // Descriptor Type:
        // 0 - reserved
        // 1 - unformatted :)
        // 2 - formatted. Here we get the used capacity (lead-in to last lead-out/border-out)
        // 3 - No media present
        //
        for( unsigned int i = 12; i < realLength-4; i+=8 ) {
            int format = (int)((buffer[i+4]>>2)&0x3f);
            qDebug() << "(K3b::Device::Device) " << blockDeviceName() << " READ FORMAT CAPACITY: "
                     << format << " " << from4Byte( &buffer[i] )
                     << " " << (int)( (buffer[i+5] << 16 & 0xFF0000) |
                                      (buffer[i+6] << 8  & 0xFF00) |
                                      (buffer[i+7]       & 0xFF) ) << endl;

            if( format == wantedFormat ) {
                // found the descriptor
                r = qMax( (int)from4Byte( &buffer[i] ), r.lba() );
                success = true;
            }
        }
    }

    return success;
}


bool K3b::Device::Device::readDiscInformation( UByteArray& data ) const
{
    unsigned char header[2];
    ::memset( header, 0, 2 );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_DISC_INFORMATION;
    cmd[8] = 2;
    cmd[9] = 0;      // Necessary to set the proper command length

    unsigned int dataLen = 0;
    if( cmd.transport( TR_DIR_READ, header, 2 ) == 0 )
        dataLen = from2Byte( header ) + 2u;
    else
        qDebug() << "(K3b::Device::Device) " << blockDeviceName()
                 << ": READ DISC INFORMATION length det failed" << endl;

    if( dataLen < 32 ) {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName()
                 << ": Device reports bogus disc information length of " << dataLen << endl;
        dataLen = 32;
    }

    data.resize( dataLen );
    ::memset( data.data(), 0, data.size() );

    cmd[7] = data.size() >> 8;
    cmd[8] = data.size();
    if( cmd.transport( TR_DIR_READ, data.data(), data.size() ) == 0 ) {
        data.resize( qMin( data.size(), from2Byte( data.data() ) + 2 ) );
        return true;
    }
    else {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ DISC INFORMATION with real length "
                 << dataLen << " failed." << endl;
        data.clear();
        return false;
    }
}


bool K3b::Device::Device::readDvdStructure( UByteArray&  data,
                                          unsigned int format,
                                          unsigned int layer,
                                          unsigned long address,
                                          unsigned int agid ) const
{
    return readDiscStructure( data, 0x0, format, layer, address, agid );
}


bool K3b::Device::Device::readDiscStructure( UByteArray& data,
                                           unsigned int mediaType,
                                           unsigned int format,
                                           unsigned int layer,
                                           unsigned long address,
                                           unsigned int agid ) const
{
    unsigned char header[4];
    ::memset( header, 0, 4 );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_DVD_STRUCTURE;
    cmd[1] = mediaType & 0xF;
    cmd[2] = address>>24;
    cmd[3] = address>>16;
    cmd[4] = address>>8;
    cmd[5] = address;
    cmd[6] = layer;
    cmd[7] = format;
    cmd[10] = (agid<<6);
    cmd[11] = 0;      // Necessary to set the proper command length

    cmd[9] = 4;
    if( cmd.transport( TR_DIR_READ, header, 4 ) == 0 ) {
        // again with real length
        unsigned int dataLen = from2Byte( header ) + 2;

        data.resize( dataLen );
        ::memset( data.data(), 0, data.size() );

        cmd[8] = data.size() >> 8;
        cmd[9] = data.size();
        if( cmd.transport( TR_DIR_READ, data.data(), data.size() ) == 0 ) {
            data.resize( qMin( data.size(), from2Byte( data.data() ) + 2 ) );
            return true;
        }
        else {
            qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ DVD STRUCTURE with real length failed.";
            data.clear();
            return false;
        }
    }
    else {
        qDebug() << "(K3b::Device::Device) " << blockDeviceName() << ": READ DVD STRUCTURE length det failed";
        return false;
    }
}


int K3b::Device::Device::readBufferCapacity( long long& bufferLength, long long& bufferAvail ) const
{
    unsigned char data[12];
    ::memset( data, 0, 12 );

    ScsiCommand cmd( this );
    cmd[0] = MMC_READ_BUFFER_CAPACITY;
    cmd[8] = 12;
    cmd[9] = 0;      // Necessary to set the proper command length
    int r = cmd.transport( TR_DIR_READ, data, 12 );
    if( r )
        return r;

    unsigned int dataLength = from2Byte( data );

    if( dataLength >= 10 ) {
        bufferLength = from4Byte( &data[4] );
        bufferAvail = from4Byte( &data[8] );
    }
    else {
        bufferAvail = bufferLength = 0;
    }

    return 0;
}
