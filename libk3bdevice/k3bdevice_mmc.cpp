/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

/**
This file contains all the MMC command implementations of the K3b device class
to make the code more readable.
**/


#include "k3bdevice.h"
#include "k3bscsicommand.h"
#include "k3bdeviceglobals.h"

#include <string.h>



bool K3bDevice::Device::getFeature( unsigned char** data, int& dataLen, unsigned int feature ) const
{
  unsigned char header[2048];
  ::memset( header, 0, 2048 );

  ScsiCommand cmd( this );
  cmd[0] = MMC_GET_CONFIGURATION;
  cmd[1] = 2;      // read only specified feature
  cmd[2] = feature>>8;
  cmd[3] = feature;
  cmd[8] = 8;      // we only read the data length first
  if( cmd.transport( TR_DIR_READ, header, 8 ) ) {
    // again with real length
    dataLen = from4Byte( header ) + 4;

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we use a high power of 2 to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( dataLen == 8 ) {
      cmd[7] = 2048>>8;
      cmd[8] = 2048;
      if( cmd.transport( TR_DIR_READ, header, 2048 ) == 0 )
	dataLen = from2Byte( header ) + 4;
    }

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 ) {
      return true;
    }
    else {
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": GET CONFIGURATION with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": GET CONFIGURATION length det failed." << endl;

  return false;
}


bool K3bDevice::Device::featureCurrent( unsigned int feature ) const
{
  unsigned char* data = 0;
  int dataLen = 0;
  if( getFeature( &data, dataLen, feature ) ) {
    bool success = false;
    if( dataLen >= 11 )
      success = ( data[8+2]&1 );  // check the current flag

    delete [] data;

    return success;
  }
  else
    return false;
}


bool K3bDevice::Device::supportsFeature( unsigned int feature ) const
{
  return featureCurrent( feature );
}


bool K3bDevice::Device::readIsrc( unsigned int track, QCString& isrc ) const
{
  unsigned char* data = 0;
  int dataLen = 0;

  if( readSubChannel( &data, dataLen, 0x3, track ) ) {
    bool isrcValid = false;

    if( dataLen >= 8+18 ) {
      isrcValid = (data[8+4]>>7 & 0x1);

      if( isrcValid ) {
	isrc = QCString( reinterpret_cast<char*>(data[8+5]), 13 );

	// TODO: check the range of the chars

      }
    }

    delete [] data;

    return isrcValid;
  }
  else
    return false;
}


bool K3bDevice::Device::readMcn( QCString& mcn ) const
{
  unsigned char* data = 0;
  int dataLen = 0;

  if( readSubChannel( &data, dataLen, 0x2, 0 ) ) {
    bool mcnValid = false;

    if( dataLen >= 8+18 ) {
      mcnValid = (data[8+4]>>7 & 0x1);

      if( mcnValid )
	mcn = QCString( reinterpret_cast<char*>(data[8+5]), 14 );
    }

    delete [] data;

    return mcnValid;
  }
  else
    return false;
}


bool K3bDevice::Device::getPerformance( unsigned char** data, int& dataLen,
					    unsigned int type,
					    unsigned int dataType,
					    unsigned int lba ) const
{
  unsigned char header[2048];
  ::memset( header, 0, 2048 );

  ScsiCommand cmd( this );
  cmd[0] = MMC_GET_PERFORMANCE;
  cmd[1] = dataType;
  cmd[2] = lba >> 24;
  cmd[3] = lba >> 16;
  cmd[4] = lba >> 8;
  cmd[5] = lba;
  cmd[9] = 1;      // first we read only the header and one descriptor
  cmd[10] = type;
  if( cmd.transport( TR_DIR_READ, header, 8 + 16 ) == 0 ) {
    // again with real length
    dataLen = from4Byte( header ) + 4;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    int numDesc = (dataLen-8)/16;

    cmd[8] = numDesc>>8;
    cmd[9] = numDesc;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 ) {
      return true;
    }
    else {
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName()
		<< ": GET PERFORMANCE with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName()
	      << ": GET PERFORMANCE length det failed." << endl;

  return false;
}


bool K3bDevice::Device::setSpeed( unsigned int readingSpeed,
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
  return ( cmd.transport() == 0 );
}


bool K3bDevice::Device::seek( unsigned long lba ) const
{
  ScsiCommand cmd( this );
  cmd[0] = MMC_SEEK_10;
  cmd[2] = lba>>24;
  cmd[3] = lba>>16;
  cmd[4] = lba>>8;
  cmd[5] = lba;

  return !cmd.transport();
}


bool K3bDevice::Device::readTrackInformation( unsigned char** data, int& dataLen, int type, unsigned long value ) const
{
  unsigned char header[2048];
  ::memset( header, 0, 2048 );

  ScsiCommand cmd( this );
  cmd[0] = MMC_READ_TRACK_INFORMATION;

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
    kdDebug() << "(K3bDevice::readTrackInformation) wrong type parameter: " << type << endl;
    return false;
  }

  cmd[8] = 4;      // first we read the header
  if( cmd.transport( TR_DIR_READ, header, 4 ) == 0 ) {
    // again with real length
    dataLen = from2Byte( header ) + 2;

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we use a high power of 2 to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( dataLen == 4 ) {
      cmd[7] = 2048>>8;
      cmd[8] = 2048;
      if( cmd.transport( TR_DIR_READ, header, 2048 ) == 0 )
	dataLen = from2Byte( header ) + 2;
    }

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 ) {
      return true;
    }
    else {
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ TRACK INFORMATION with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ TRACK INFORMATION length det failed." << endl;

  return false;
}



bool K3bDevice::Device::read10( unsigned char* data,
				    int dataLen,
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

  if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ 10 failed!" << endl;
    return false;
  }
  else
    return true;
}


bool K3bDevice::Device::read12( unsigned char* data,
				    int dataLen,
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

  if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ 12 failed!" << endl;
    return false;
  }
  else
    return true;
}


bool K3bDevice::Device::readCd( unsigned char* data,
				    int dataLen,
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

  if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ CD failed!" << endl;
    return false;
  }
  else
    return true;
}


bool K3bDevice::Device::readCdMsf( unsigned char* data,
				   int dataLen,
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

  if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ CD MSF failed!" << endl;
    return false;
  }
  else
    return true;
}


bool K3bDevice::Device::readSubChannel( unsigned char** data, int& dataLen,
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
  cmd[8] = 4;      // first we read the header
  if( cmd.transport( TR_DIR_READ, header, 4 ) == 0 ) {
    // again with real length
    dataLen = from2Byte( &header[2] ) + 4;

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we use a high power of 2 to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( dataLen == 4 ) {
      cmd[7] = 2048>>8;
      cmd[8] = 2048;
      if( cmd.transport( TR_DIR_READ, header, 2048 ) == 0 )
	dataLen = from2Byte( &header[2] ) + 4;
    }

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 ) {
      return true;
    }
    else {
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ SUB-CHANNEL with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ SUB-CHANNEL length det failed." << endl;

  return false;
}


bool K3bDevice::Device::readTocPmaAtip( unsigned char** data, int& dataLen, int format, bool time, int track ) const
{
  unsigned char header[2048];
  ::memset( header, 0, 2048 );

  ScsiCommand cmd( this );
  cmd[0] = MMC_READ_TOC_PMA_ATIP;
  cmd[1] = ( time ? 0x2 : 0x0 );
  cmd[2] = format & 0x0F;
  cmd[6] = track;
  cmd[8] = 2; // we only read the length first

  if( cmd.transport( TR_DIR_READ, header, 2 ) == 0 ) {
    // again with real length
    dataLen = from2Byte( header ) + 2;

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we use a high power of 2 to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( dataLen == 2 ) {
      cmd[7] = 2048>>8;
      cmd[8] = 2048;
      if( cmd.transport( TR_DIR_READ, header, 2048 ) == 0 )
	dataLen = from2Byte( header ) + 2;
    }

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 )
      return true;
    else {
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ TOC/PMA/ATIP format "
		<< format << " with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ TOC/PMA/ATIP length det failed." << endl;

  return false;
}


bool K3bDevice::Device::mechanismStatus( unsigned char** data, int& dataLen ) const
{
  unsigned char header[2048];
  ::memset( header, 0, 2048 );

  ScsiCommand cmd( this );
  cmd[0] = MMC_MECHANISM_STATUS;
  cmd[9] = 8;     // first we read the header
  if( cmd.transport( TR_DIR_READ, header, 8 ) == 0 ) {
    // again with real length
    dataLen = from4Byte( &header[6] ) + 8;

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we use a high power of 2 to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( dataLen == 8 ) {
      cmd[8] = 2048>>8;
      cmd[9] = 2048;
      if( cmd.transport( TR_DIR_READ, header, 2048 ) == 0 )
	dataLen = from2Byte( &header[6] ) + 8;
    }

    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": MECHANISM STATUS "
	      << (int)header[5] << " slots." << endl;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[8] = dataLen>>8;
    cmd[9] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 ) {
      return true;
    }
    else {
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": MECHANISM STATUS with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": MECHANISM STATUS length det failed." << endl;

  return false;
}



bool K3bDevice::Device::modeSense( unsigned char** pageData, int& pageLen, int page ) const
{
  unsigned char header[2048];
  ::memset( header, 0, 2048 );

  ScsiCommand cmd( this );
  cmd[0] = MMC_MODE_SENSE;
  cmd[1] = 0x08;        // Disable Block Descriptors
  cmd[2] = page;
  cmd[8] = 8;           // first we determine the data length
  if( cmd.transport( TR_DIR_READ, header, 8 ) == 0 ) {
    // again with real length
    pageLen = from2Byte( header ) + 2;

    //
    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we use a high power of 2 to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    //
    if( pageLen == 8 ) {
      cmd[7] = 2048>>8;
      cmd[8] = 2048;
      if( cmd.transport( TR_DIR_READ, header, 2048 ) == 0 )
	pageLen = from2Byte( header ) + 2;
    }

    *pageData = new unsigned char[pageLen];
    ::memset( *pageData, 0, pageLen );

    cmd[7] = pageLen>>8;
    cmd[8] = pageLen;
    if( cmd.transport( TR_DIR_READ, *pageData, pageLen ) == 0 )
      return true;
    else {
      delete [] *pageData;
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": MODE SENSE with real length "
		<< pageLen << " failed." << endl;
    }
  }
  else
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": MODE SENSE length det failed." << endl;

  return false;
}


bool K3bDevice::Device::modeSelect( unsigned char* page, int pageLen, bool pf, bool sp ) const
{
  page[0] = 0;
  page[1] = 0;
  page[4] = 0;
  page[5] = 0;

  // we do not support Block Descriptors here
  page[6] = 0;
  page[7] = 0;

  ScsiCommand cmd( this );
  cmd[0] = MMC_MODE_SELECT;
  cmd[1] = ( sp ? 1 : 0 ) | ( pf ? 0x10 : 0 );
  cmd[7] = pageLen>>8;
  cmd[8] = pageLen;
  cmd[9] = 0;
  return( cmd.transport( TR_DIR_WRITE, page, pageLen ) == 0 );
}


// does only make sense for complete media
bool K3bDevice::Device::readCapacity( K3b::Msf& r ) const
{
  ScsiCommand cmd( this );
  cmd[0] = MMC_READ_CAPACITY;
  unsigned char buf[8];
  ::memset( buf, 0, 8 );
  if( cmd.transport( TR_DIR_READ, buf, 8 ) == 0 ) {
    r = from4Byte( buf );
    return true;
  }
  else
    return false;
}


bool K3bDevice::Device::readFormatCapacity( int wantedFormat, K3b::Msf& r,
					    K3b::Msf* currentMax, int* currentMaxFormat ) const
{
  bool success = false;

  // the maximal length as stated in MMC4
  static const unsigned int maxLen = 4 + (8*31);

  unsigned char buffer[maxLen];
  ::memset( buffer, 0, maxLen );

  ScsiCommand cmd( this );
  cmd[0] = MMC_READ_FORMAT_CAPACITIES;
  cmd[7] = maxLen >> 8;
  cmd[8] = maxLen & 0xFF;
  if( cmd.transport( TR_DIR_READ, buffer, maxLen ) == 0 ) {

    int realLength = buffer[3] + 4;

    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << " READ FORMAT CAPACITY: Current/Max "
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
    for( int i = 12; i < realLength-4; i+=8 ) {
      int format = (int)((buffer[i+4]>>2)&0x3f);
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << " READ FORMAT CAPACITY: "
		<< format << " " << from4Byte( &buffer[i] )
		<< " " << (int)( buffer[i+5] << 16 & 0xFF0000 |
				 buffer[i+6] << 8  & 0xFF00 |
				 buffer[i+7]       & 0xFF ) << endl;

      if( format == wantedFormat ) {
	// found the descriptor
	r = QMAX( (int)from4Byte( &buffer[i] ), r.lba() );
	success = true;
      }
    }
  }

  return success;
}


bool K3bDevice::Device::readDiscInfo( unsigned char** data, int& dataLen ) const
{
  unsigned char header[2];
  ::memset( header, 0, 2 );

  ScsiCommand cmd( this );
  cmd[0] = MMC_READ_DISK_INFORMATION;
  cmd[8] = 2;

  if( cmd.transport( TR_DIR_READ, header, 2 ) == 0 ) {
    // again with real length
    dataLen = from2Byte( header ) + 2;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 )
      return true;
    else {
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ DISC INFORMATION with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else {
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ DISC INFORMATION length det failed" << endl;
  }

  return false;
}


bool K3bDevice::Device::readDvdStructure( unsigned char** data, int& dataLen, 
					  unsigned int format,
					  unsigned int layer,
					  unsigned long adress,
					  unsigned int agid ) const
{
  unsigned char header[4];
  ::memset( header, 0, 4 );

  ScsiCommand cmd( this );
  cmd[0] = MMC_READ_DVD_STRUCTURE;
  cmd[2] = adress>>24;
  cmd[3] = adress>>16;
  cmd[4] = adress>>8;
  cmd[5] = adress;
  cmd[6] = layer;
  cmd[7] = format;
  cmd[10] = (agid<<6);

  cmd[9] = 4;
  if( cmd.transport( TR_DIR_READ, header, 2 ) == 0 ) {
    // again with real length
    dataLen = from2Byte( header ) + 2;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[8] = dataLen>>8;
    cmd[9] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 )
      return true;
    else {
      kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ DVD STRUCTURE with real length failed." << endl;
    }
  }
  else
    kdDebug() << "(K3bDevice::Device) " << blockDeviceName() << ": READ DVD STRUCTURE length det failed" << endl;

  return false;
}
