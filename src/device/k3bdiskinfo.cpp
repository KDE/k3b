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

#include "k3bdiskinfo.h"
#include "k3bdeviceglobals.h"

#include <k3bmsf.h>

#include <klocale.h>
#include <kdebug.h>
#include <kio/global.h>

#include <qstringlist.h>


K3bCdDevice::DiskInfo::DiskInfo()
  : mediaType(0),
    empty(false),
    cdrw(false),
    appendable(false),
    noDisk(true),
    isVideoDvd(false),
    isVCD(false),
    size(0),
    remaining(0),
    speed(0),
    sessions(0),
    tocType(UNKNOWN),
    valid(false),
    device(0)
{
}


K3bCdDevice::NextGenerationDiskInfo::NextGenerationDiskInfo()
  : m_mediaType(MEDIA_UNKNOWN),
    m_currentProfile(MEDIA_UNKNOWN),
    m_diskState(STATE_UNKNOWN),
    m_lastSessionState(STATE_UNKNOWN),
    m_bgFormatState(0),
    m_numSessions(0),
    m_numTracks(0),
    m_rewritable(false)
{
}


K3bCdDevice::NextGenerationDiskInfo::~NextGenerationDiskInfo()
{
}


int K3bCdDevice::NextGenerationDiskInfo::diskState() const
{
  return m_diskState;
}


int K3bCdDevice::NextGenerationDiskInfo::lastSessionState() const
{
  return m_lastSessionState;
}


int K3bCdDevice::NextGenerationDiskInfo::bgFormatState() const
{
  return m_bgFormatState;
}


bool K3bCdDevice::NextGenerationDiskInfo::empty() const
{
  return diskState() == STATE_EMPTY;
}


bool K3bCdDevice::NextGenerationDiskInfo::rewritable() const
{
  return m_rewritable;
}


bool K3bCdDevice::NextGenerationDiskInfo::appendable() const
{
  return diskState() == STATE_INCOMPLETE;
}


int K3bCdDevice::NextGenerationDiskInfo::mediaType() const
{
  return m_mediaType;
}


bool K3bCdDevice::NextGenerationDiskInfo::isDvdMedia() const
{
  return K3bCdDevice::isDvdMedia( mediaType() );
}


int K3bCdDevice::NextGenerationDiskInfo::numSessions() const
{
  if( empty() )
    return 0;
  else
    return m_numSessions;
}


int K3bCdDevice::NextGenerationDiskInfo::numTracks() const
{
  if( empty() )
    return 0;
  else
    return m_numTracks;
}


K3bMsf K3bCdDevice::NextGenerationDiskInfo::remainingSize() const
{
  if( empty() )
    return capacity();
  else if( appendable() )
    return capacity() - m_usedCapacity;

  //
  // There is no way to properly determine the used size on an overwrite media
  // without having a look at the filesystem (or is there?)
  //
  else if( mediaType() & (MEDIA_DVD_PLUS_RW|MEDIA_DVD_RW_OVWR) )
    return capacity();
  else
    return 0;
}


K3bMsf K3bCdDevice::NextGenerationDiskInfo::capacity() const
{
  return (m_capacity == 0 ? size() : m_capacity);
}


K3bMsf K3bCdDevice::NextGenerationDiskInfo::size() const
{
  return m_usedCapacity;
}


void K3bCdDevice::NextGenerationDiskInfo::debug() const
{
  kdDebug() << "NextGenerationDiskInfo:" << endl
	    << "Mediatype:       " << K3bCdDevice::mediaTypeString( mediaType() ) << endl
	    << "Current Profile: " << K3bCdDevice::mediaTypeString( currentProfile() ) << endl
	    << "Disk state:      " << ( diskState() == K3bCdDevice::STATE_EMPTY ? 
					"empty" :
					( diskState() == K3bCdDevice::STATE_INCOMPLETE ?
					  "incomplete" :
					  ( diskState() == K3bCdDevice::STATE_COMPLETE ?
					    "complete" : 
					    ( diskState() == K3bCdDevice::STATE_NO_MEDIA ?
					      "no media" : 
					      "unknown" ) ) ) ) << endl
	    << "Empty:           " << empty() << endl
	    << "Rewritable:      " << rewritable() << endl
	    << "Appendable:      " << appendable() << endl
	    << "Sessions:        " << numSessions() << endl
	    << "Tracks:          " << numTracks() << endl
	    << "Capacity:        " << capacity().toString() 
	    << " (LBA " << QString::number(capacity().lba())
	    << ") (" << QString::number(capacity().mode1Bytes()) << " Bytes) (" 
	    << KIO::convertSize(capacity().mode1Bytes()) << ")" << endl

	    << "Remaining size:  " << remainingSize().toString() 
	    << " (LBA " << QString::number(remainingSize().lba())
	    << ") (" << QString::number(remainingSize().mode1Bytes()) << " Bytes) (" 
	    << KIO::convertSize(remainingSize().mode1Bytes()) << ")" << endl

	    << "Used Size:       " << size().toString()  
	    << " (LBA " << QString::number(size().lba())
	    << ") (" << QString::number(size().mode1Bytes()) << " Bytes) (" 
	    << KIO::convertSize(size().mode1Bytes()) << ")" << endl;

  if( mediaType() == K3bCdDevice::MEDIA_DVD_PLUS_RW )
    kdDebug() << "Bg Format:       " << ( bgFormatState() == BG_FORMAT_NONE ? 
					  "none" :
					  ( bgFormatState() == BG_FORMAT_INCOMPLETE ?
					    "incomplete" :
					    ( bgFormatState() == BG_FORMAT_IN_PROGRESS ?
					      "in progress" :
					      "complete" ) ) ) << endl;
}


// kdbgstream& K3bCdDevice::operator<<( kdbgstream& s, const K3bCdDevice::NextGenerationDiskInfo& ngInf )
// {
//    s << "NextGenerationDiskInfo:" << endl
//      << "Mediatype:       " << K3bCdDevice::mediaTypeString( ngInf.mediaType() ) << endl
//      << "Current Profile: " << K3bCdDevice::mediaTypeString( ngInf.currentProfile() ) << endl
//      << "Disk state:      " << ( ngInf.diskState() == K3bCdDevice::STATE_EMPTY ? 
// 				 "empty" :
// 				 ( ngInf.diskState() == K3bCdDevice::STATE_INCOMPLETE ?
// 				   "incomplete" :
// 				   ( ngInf.diskState() == K3bCdDevice::STATE_COMPLETE ?
// 				     "complete" : 
// 				     ( ngInf.diskState() == K3bCdDevice::STATE_NO_MEDIA ?
// 				       "no media" : 
// 				       "unknown" ) ) ) ) << endl
//      << "Empty:           " << ngInf.empty() << endl
//      << "Rewritable:      " << ngInf.rewritable() << endl
//      << "Appendable:      " << ngInf.appendable() << endl
//      << "Sessions:        " << ngInf.numSessions() << endl
//      << "Tracks:          " << ngInf.numTracks() << endl
//      << "Size:            " << ngInf.capacity().toString() << endl
//      << "Remaining size:  " << ngInf.remainingSize().toString() << endl;
   
//    return s;
// }
