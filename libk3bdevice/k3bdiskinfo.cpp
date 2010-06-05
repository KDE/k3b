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

#include "k3bdiskinfo.h"
#include "k3bdiskinfo_p.h"
#include "k3bdeviceglobals.h"

#include "k3bmsf.h"

#include <klocale.h>
#include <kdebug.h>
#include <kio/global.h>

#include <qstringlist.h>


K3b::Device::DiskInfo::DiskInfo()
    : d( new DiskInfoPrivate() )
{
}


K3b::Device::DiskInfo::DiskInfo( const DiskInfo& other )
{
    d = other.d;
}


K3b::Device::DiskInfo::~DiskInfo()
{
}


K3b::Device::DiskInfo& K3b::Device::DiskInfo::operator=( const DiskInfo& other )
{
    d = other.d;
    return *this;
}


K3b::Device::MediaState K3b::Device::DiskInfo::diskState() const
{
    return d->diskState;
}


K3b::Device::MediaState K3b::Device::DiskInfo::lastSessionState() const
{
    return d->lastSessionState;
}


K3b::Device::BackGroundFormattingState K3b::Device::DiskInfo::bgFormatState() const
{
    return d->bgFormatState;
}


bool K3b::Device::DiskInfo::empty() const
{
    return diskState() == STATE_EMPTY;
}


bool K3b::Device::DiskInfo::rewritable() const
{
    return d->rewritable;
}


bool K3b::Device::DiskInfo::appendable() const
{
    return diskState() == STATE_INCOMPLETE;
}


K3b::Device::MediaType K3b::Device::DiskInfo::mediaType() const
{
    return d->mediaType;
}


int K3b::Device::DiskInfo::currentProfile() const
{
    return d->currentProfile;
}


QByteArray K3b::Device::DiskInfo::mediaId() const
{
    return d->mediaId;
}


int K3b::Device::DiskInfo::numSessions() const
{
    if( empty() )
        return 0;
    else
        return d->numSessions;
}


int K3b::Device::DiskInfo::numTracks() const
{
    if( empty() )
        return 0;
    else
        return d->numTracks;
}


int K3b::Device::DiskInfo::numLayers() const
{
    if( isDvdMedia( mediaType() ) )
        return d->numLayers;
    else
        return 1;
}


K3b::Msf K3b::Device::DiskInfo::remainingSize() const
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


K3b::Msf K3b::Device::DiskInfo::capacity() const
{
    return (d->capacity == 0 ? size() : d->capacity);
}


K3b::Msf K3b::Device::DiskInfo::size() const
{
    if( empty() )
        return 0;
    else
        return d->usedCapacity;
}


K3b::Msf K3b::Device::DiskInfo::firstLayerSize() const
{
    if( numLayers() > 1 )
        return d->firstLayerSize;
    else
        return size();
}


void K3b::Device::DiskInfo::debug() const
{
    kDebug() << "DiskInfo:" << endl
             << "Mediatype:       " << K3b::Device::mediaTypeString( mediaType() ) << endl
             << "Current Profile: " << K3b::Device::mediaTypeString( currentProfile() ) << endl
             << "Disk state:      " << ( diskState() == K3b::Device::STATE_EMPTY ?
                                         "empty" :
                                         ( diskState() == K3b::Device::STATE_INCOMPLETE ?
                                           "incomplete" :
                                           ( diskState() == K3b::Device::STATE_COMPLETE ?
                                             "complete" :
                                             ( diskState() == K3b::Device::STATE_NO_MEDIA ?
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

    if( mediaType() == K3b::Device::MEDIA_DVD_PLUS_RW )
        kDebug() << "Bg Format:       " << ( bgFormatState() == BG_FORMAT_NONE ?
                                             "none" :
                                             ( bgFormatState() == BG_FORMAT_INCOMPLETE ?
                                               "incomplete" :
                                               ( bgFormatState() == BG_FORMAT_IN_PROGRESS ?
                                                 "in progress" :
                                                 ( bgFormatState() == BG_FORMAT_COMPLETE ?
                                                   "complete" : "unknown" ) ) ) ) << endl;
}


bool K3b::Device::DiskInfo::operator==( const K3b::Device::DiskInfo& other ) const
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


bool K3b::Device::DiskInfo::operator!=( const K3b::Device::DiskInfo& other ) const
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


// kdbgstream& K3b::Device::operator<<( kdbgstream& s, const K3b::Device::DiskInfo& ngInf )
// {
//    s << "DiskInfo:" << endl
//      << "Mediatype:       " << K3b::Device::mediaTypeString( ngInf.mediaType() ) << endl
//      << "Current Profile: " << K3b::Device::mediaTypeString( ngInf.currentProfile() ) << endl
//      << "Disk state:      " << ( ngInf.diskState() == K3b::Device::STATE_EMPTY ?
// 				 "empty" :
// 				 ( ngInf.diskState() == K3b::Device::STATE_INCOMPLETE ?
// 				   "incomplete" :
// 				   ( ngInf.diskState() == K3b::Device::STATE_COMPLETE ?
// 				     "complete" :
// 				     ( ngInf.diskState() == K3b::Device::STATE_NO_MEDIA ?
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
