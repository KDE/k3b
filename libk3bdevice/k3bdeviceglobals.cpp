/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdeviceglobals.h"
#include "k3bdiskinfo.h"
#include "k3bdevice.h"

#include <klocale.h>
#include <kdebug.h>

#include <qstringlist.h>


QString K3bDevice::deviceTypeString( int t )
{
  QStringList s;
  if( t & K3bDevice::CDR )
    s += i18n("CD-R");
  if( t & K3bDevice::CDRW )
    s += i18n("CD-RW");
  if( t & K3bDevice::CDROM )
    s += i18n("CD-ROM");
  if( t & K3bDevice::DVD )
    s += i18n("DVD-ROM");
  if( t & K3bDevice::DVDRAM )
    s += i18n("DVD-RAM");
  if( t & K3bDevice::DVDR )
    s += i18n("DVD-R");
  if( t & K3bDevice::DVDRW )
    s += i18n("DVD-RW");
  if( t & K3bDevice::DVDPR )
    s += i18n("DVD+R");
  if( t & K3bDevice::DVDPRW )
    s += i18n("DVD+RW");

  if( s.isEmpty() )
    return i18n("Error");
  else
    return s.join( "; " );
}


QString K3bDevice::writingModeString( int m )
{
  QStringList s;
  if( m & K3bDevice::SAO )
    s += i18n("SAO");
  if( m & K3bDevice::TAO )
    s += i18n("TAO");
  if( m & K3bDevice::RAW )
    s += i18n("RAW");
  if( m & K3bDevice::PACKET )
    s += i18n("PACKET");
  if( m & K3bDevice::SAO_R96P )
    s += i18n("SAO/R96P");
  if( m & K3bDevice::SAO_R96R )
    s += i18n("SAO/R96R");
  if( m & K3bDevice::RAW_R16 )
    s += i18n("SAO/R16");
  if( m & K3bDevice::RAW_R96P )
    s += i18n("RAW/R96P");
  if( m & K3bDevice::RAW_R96R )
    s += i18n("RAW/R96R");

  if( s.isEmpty() )
    return i18n("None");
  else
    return s.join( "; " );
}


QString K3bDevice::mediaTypeString( int m, bool simple )
{
  if( m == K3bDevice::MEDIA_UNKNOWN )
    return i18n("Unknown");

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
  if( m & MEDIA_DVD_PLUS_R_DL )
    s += i18n("DVD+R Double Layer");
  if( m & MEDIA_CD_ROM )
    s += i18n("CD-ROM");
  if( m & MEDIA_CD_R )
    s += i18n("CD-R");
  if( m & MEDIA_CD_RW )
    s += i18n("CD-RW");

  if( s.isEmpty() )
    return i18n("Error");
  else
    return s.join( "; " );
}


void K3bDevice::debugBitfield( unsigned char* data, long len )
{
  for( int i = 0; i < len; ++i ) {
    QString index, bitString;
    index.sprintf( "%4i", i );
    for( int bp = 7; bp >= 0; --bp )
      bitString[7-bp] = ( data[i] & (1<<bp) ? '1' : '0' );
    kdDebug() << index << " - " << bitString << " - " << (int)data[i] << endl;
  }
}


unsigned short K3bDevice::from2Byte( unsigned char* d )
{
  return ( d[0] << 8 & 0xFF00 |
	   d[1]      & 0xFF );
}


unsigned long K3bDevice::from4Byte( unsigned char* d )
{
  return ( d[0] << 24 & 0xFF000000 |
	   d[1] << 16 & 0xFF0000 |
	   d[2] << 8  & 0xFF00 |
	   d[3]       & 0xFF );
}


char K3bDevice::fromBcd( const char& i )
{
  return (i & 0x0f) + 10 * ( (i >> 4) & 0x0f );
}


char K3bDevice::toBcd( const char& i )
{
  return ( i % 10 ) | ( ( (( i / 10 ) % 10) << 4 ) & 0xf0 );
}


bool K3bDevice::isValidBcd( const char& i )
{
  return ( i & 0x0f ) <= 0x09 && ( i & 0xf0 ) <= 0x90;
}


int K3bDevice::determineMaxReadingBufferSize( K3bDevice::Device* dev, const K3b::Msf& firstSector )
{
  //
  // As long as we do not know how to determine the max read buffer properly we simply determine it
  // by trying. :)
  //

  int bufferSizeSectors = 128;
  unsigned char buffer[2048*128];
  while( !dev->read10( buffer, 2048*bufferSizeSectors, firstSector.lba(), bufferSizeSectors ) ) {
    kdDebug() << "(K3bDataTrackReader) determine max read sectors: "
	      << bufferSizeSectors << " too high." << endl;
    bufferSizeSectors--;
  }
  kdDebug() << "(K3bDataTrackReader) determine max read sectors: " 
	    << bufferSizeSectors << " is max." << endl;

  return bufferSizeSectors;
}
