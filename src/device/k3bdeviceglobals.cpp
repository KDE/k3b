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


QString K3bCdDevice::deviceTypeString( int t )
{
  QStringList s;
  if( t & K3bCdDevice::CdDevice::CDR )
    s += i18n("CD-R");
  if( t & K3bCdDevice::CdDevice::CDRW )
    s += i18n("CD-RW");
  if( t & K3bCdDevice::CdDevice::CDROM )
    s += i18n("CD-ROM");
  if( t & K3bCdDevice::CdDevice::DVD )
    s += i18n("DVD-ROM");
  if( t & K3bCdDevice::CdDevice::DVDRAM )
    s += i18n("DVD-RAM");
  if( t & K3bCdDevice::CdDevice::DVDR )
    s += i18n("DVD-R");
  if( t & K3bCdDevice::CdDevice::DVDRW )
    s += i18n("DVD-RW");
  if( t & K3bCdDevice::CdDevice::DVDPR )
    s += i18n("DVD+R");
  if( t & K3bCdDevice::CdDevice::DVDPRW )
    s += i18n("DVD+RW");

  if( s.isEmpty() )
    return i18n("Error");
  else
    return s.join( "; " );
}


QString K3bCdDevice::writingModeString( int m )
{
  QStringList s;
  if( m & K3bCdDevice::CdDevice::SAO )
    s += i18n("SAO");
  if( m & K3bCdDevice::CdDevice::TAO )
    s += i18n("TAO");
  if( m & K3bCdDevice::CdDevice::RAW )
    s += i18n("RAW");
  if( m & K3bCdDevice::CdDevice::PACKET )
    s += i18n("PACKET");
  if( m & K3bCdDevice::CdDevice::SAO_R96P )
    s += i18n("SAO/R96P");
  if( m & K3bCdDevice::CdDevice::SAO_R96R )
    s += i18n("SAO/R96R");
  if( m & K3bCdDevice::CdDevice::RAW_R16 )
    s += i18n("SAO/R16");
  if( m & K3bCdDevice::CdDevice::RAW_R96P )
    s += i18n("RAW/R96P");
  if( m & K3bCdDevice::CdDevice::RAW_R96R )
    s += i18n("RAW/R96R");

  if( s.isEmpty() )
    return i18n("None");
  else
    return s.join( "; " );
}


QString K3bCdDevice::mediaTypeString( int m )
{
  if( m == -1 )
    return i18n("Error");

  QStringList s;
  if( m & MEDIA_NONE )
    s += i18n("No media");
  if( m & MEDIA_DVD_ROM )
    s += i18n("DVD-ROM");
  if( m & MEDIA_DVD_R )
    s += i18n("DVD-R");
  if( m & MEDIA_DVD_R_SEQ )
    s += i18n("DVD-R Sequential");
  if( m & MEDIA_DVD_RAM )
    s += i18n("DVD-RAM");
  if( m & MEDIA_DVD_RW )
    s += i18n("DVD-RW");
  if( m & MEDIA_DVD_RW_OVWR )
    s += i18n("DVD-RW Restricted Overwrite");
  if( m & MEDIA_DVD_RW_SEQ )
    s += i18n("DVD-RW Sequential");
  if( m & MEDIA_DVD_PLUS_RW )
    s += i18n("DVD+RW");
  if( m & MEDIA_DVD_PLUS_R )
    s += i18n("DVD+R");
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


void K3bCdDevice::debugBitfield( unsigned char* data, long len )
{
  for( int i = 0; i < len; ++i ) {
    QString index, bitString;
    index.sprintf( "%4i", i );
    for( int bp = 7; bp >= 0; --bp )
      bitString[7-bp] = ( data[i] & (1<<bp) ? '1' : '0' );
    kdDebug() << index << " - " << bitString << " - " << (int)data[i] << endl;
  }
}


unsigned short K3bCdDevice::from2Byte( unsigned char* d )
{
  return ( d[0] << 8 & 0xFF00 |
	   d[1]      & 0xFF );
}


unsigned long K3bCdDevice::from4Byte( unsigned char* d )
{
  return ( d[0] << 24 & 0xFF000000 |
	   d[1] << 16 & 0xFF0000 |
	   d[2] << 8  & 0xFF00 |
	   d[3]       & 0xFF );
}
