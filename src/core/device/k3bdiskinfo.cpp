/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdiskinfo.h"

#include <k3bmsf.h>

#include <klocale.h>
#include <kdebug.h>

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


QString K3bCdDevice::mediaTypeString( int m )
{
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



K3bCdDevice::NextGenerationDiskInfo::NextGenerationDiskInfo()
  : m_mediaType(-1),
    m_currentProfile(-1),
    m_diskState(-1),
    m_lastSessionState(-1),
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
  else
    return m_remaining;
}


K3bMsf K3bCdDevice::NextGenerationDiskInfo::capacity() const
{
  return m_capacity;
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
	    << "Size:            " << capacity().toString() << endl
	    << "Remaining size:  " << remainingSize().toString() << endl;
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
