/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdiskinfo.h"
#include "k3bdiskinfo_p.h"
#include "k3bdeviceglobals.h"

#include <k3bmsf.h>

#include <klocale.h>
#include <kdebug.h>
#include <kio/global.h>

#include <qstringlist.h>


K3bDevice::DiskInfo::DiskInfo()
    : d( new DiskInfoPrivate() )
{
}


K3bDevice::DiskInfo::DiskInfo( const DiskInfo& other )
{
    d = other.d;
}


K3bDevice::DiskInfo::~DiskInfo()
{
}


K3bDevice::DiskInfo& K3bDevice::DiskInfo::operator=( const DiskInfo& other )
{
    d = other.d;
    return *this;
}


int K3bDevice::DiskInfo::diskState() const
{
    return d->diskState;
}


int K3bDevice::DiskInfo::lastSessionState() const
{
    return d->lastSessionState;
}


int K3bDevice::DiskInfo::bgFormatState() const
{
    return d->bgFormatState;
}


bool K3bDevice::DiskInfo::empty() const
{
    return diskState() == STATE_EMPTY;
}


bool K3bDevice::DiskInfo::rewritable() const
{
    return d->rewritable;
}


bool K3bDevice::DiskInfo::appendable() const
{
    return diskState() == STATE_INCOMPLETE;
}


int K3bDevice::DiskInfo::mediaType() const
{
    return d->mediaType;
}


bool K3bDevice::DiskInfo::isDvdMedia() const
{
    return K3bDevice::isDvdMedia( mediaType() );
}


int K3bDevice::DiskInfo::currentProfile() const
{
    return d->currentProfile;
}


QByteArray K3bDevice::DiskInfo::mediaId() const
{
    return d->mediaId;
}


int K3bDevice::DiskInfo::numSessions() const
{
    if( empty() )
        return 0;
    else
        return d->numSessions;
}


int K3bDevice::DiskInfo::numTracks() const
{
    if( empty() )
        return 0;
    else
        return d->numTracks;
}


int K3bDevice::DiskInfo::numLayers() const
{
    if( isDvdMedia() )
        return d->numLayers;
    else
        return 1;
}


K3b::Msf K3bDevice::DiskInfo::remainingSize() const
{
    if( empty() )
        return capacity();

    //
    // There is no way to properly determine the used size on an overwrite media
    // without having a look at the filesystem (or is there?)
    //
    else if( appendable() ||
             mediaType() & (MEDIA_DVD_PLUS_RW|MEDIA_DVD_RW_OVWR) )
        return capacity() - d->usedCapacity;

    else
        return 0;
}


K3b::Msf K3bDevice::DiskInfo::capacity() const
{
    return (d->capacity == 0 ? size() : d->capacity);
}


K3b::Msf K3bDevice::DiskInfo::size() const
{
    if( empty() )
        return 0;
    else
        return d->usedCapacity;
}


K3b::Msf K3bDevice::DiskInfo::firstLayerSize() const
{
    if( numLayers() > 1 )
        return d->firstLayerSize;
    else
        return size();
}


void K3bDevice::DiskInfo::debug() const
{
    kDebug() << "DiskInfo:" << endl
             << "Mediatype:       " << K3bDevice::mediaTypeString( mediaType() ) << endl
             << "Current Profile: " << K3bDevice::mediaTypeString( currentProfile() ) << endl
             << "Disk state:      " << ( diskState() == K3bDevice::STATE_EMPTY ?
                                         "empty" :
                                         ( diskState() == K3bDevice::STATE_INCOMPLETE ?
                                           "incomplete" :
                                           ( diskState() == K3bDevice::STATE_COMPLETE ?
                                             "complete" :
                                             ( diskState() == K3bDevice::STATE_NO_MEDIA ?
                                               "no media" :
                                               "unknown" ) ) ) ) << endl
             << "Empty:           " << empty() << endl
             << "Rewritable:      " << rewritable() << endl
             << "Appendable:      " << appendable() << endl
             << "Sessions:        " << numSessions() << endl
             << "Tracks:          " << numTracks() << endl
             << "Layers:          " << numLayers() << endl
             << "Capacity:        " << capacity()
             << " (LBA " << capacity().lba()
             << ") (" << capacity().mode1Bytes() << " Bytes)" << endl

             << "Remaining size:  " << remainingSize()
             << " (LBA " << remainingSize().lba()
             << ") (" << remainingSize().mode1Bytes() << " Bytes)" << endl

             << "Used Size:       " << size()
             << " (LBA " << size().lba()
             << ") (" << size().mode1Bytes() << " Bytes)" << endl;

    if( mediaType() == K3bDevice::MEDIA_DVD_PLUS_RW )
        kDebug() << "Bg Format:       " << ( bgFormatState() == BG_FORMAT_NONE ?
                                             "none" :
                                             ( bgFormatState() == BG_FORMAT_INCOMPLETE ?
                                               "incomplete" :
                                               ( bgFormatState() == BG_FORMAT_IN_PROGRESS ?
                                                 "in progress" :
                                                 ( bgFormatState() == BG_FORMAT_COMPLETE ?
                                                   "complete" : "unknown" ) ) ) ) << endl;
}


bool K3bDevice::DiskInfo::operator==( const K3bDevice::DiskInfo& other ) const
{
    return( d->mediaType == other.d->mediaType &&
            d->currentProfile == other.d->currentProfile &&
            d->diskState == other.d->diskState &&
            d->lastSessionState == other.d->lastSessionState &&
            d->bgFormatState == other.d->bgFormatState &&
            d->numSessions == other.d->numSessions &&
            d->numTracks == other.d->numTracks &&
            d->numLayers == other.d->numLayers &&
            d->rewritable == other.d->rewritable &&
            d->capacity == other.d->capacity &&
            d->usedCapacity == other.d->usedCapacity &&
            d->firstLayerSize == other.d->firstLayerSize &&
            d->mediaId == other.d->mediaId );
}


bool K3bDevice::DiskInfo::operator!=( const K3bDevice::DiskInfo& other ) const
{
    return( d->mediaType != other.d->mediaType ||
            d->currentProfile != other.d->currentProfile ||
            d->diskState != other.d->diskState ||
            d->lastSessionState != other.d->lastSessionState ||
            d->bgFormatState != other.d->bgFormatState ||
            d->numSessions != other.d->numSessions ||
            d->numTracks != other.d->numTracks ||
            d->numLayers != other.d->numLayers ||
            d->rewritable != other.d->rewritable ||
            d->capacity != other.d->capacity ||
            d->usedCapacity != other.d->usedCapacity ||
            d->firstLayerSize != other.d->firstLayerSize ||
            d->mediaId != other.d->mediaId );
}


// kdbgstream& K3bDevice::operator<<( kdbgstream& s, const K3bDevice::DiskInfo& ngInf )
// {
//    s << "DiskInfo:" << endl
//      << "Mediatype:       " << K3bDevice::mediaTypeString( ngInf.mediaType() ) << endl
//      << "Current Profile: " << K3bDevice::mediaTypeString( ngInf.currentProfile() ) << endl
//      << "Disk state:      " << ( ngInf.diskState() == K3bDevice::STATE_EMPTY ?
// 				 "empty" :
// 				 ( ngInf.diskState() == K3bDevice::STATE_INCOMPLETE ?
// 				   "incomplete" :
// 				   ( ngInf.diskState() == K3bDevice::STATE_COMPLETE ?
// 				     "complete" :
// 				     ( ngInf.diskState() == K3bDevice::STATE_NO_MEDIA ?
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
