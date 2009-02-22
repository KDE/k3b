
/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include <config-k3b.h>

#include "k3bmedium.h"
#include "k3bmedium_p.h"

#include "k3bcddb.h"
#include "k3bdeviceglobals.h"
#include "k3bglobals.h"
#include "k3biso9660.h"
#include "k3biso9660backend.h"

#include <klocale.h>
#include <kio/global.h>

#include <QtCore/QSharedData>
#include <QtCore/QList>

#include <libkcddb/cdinfo.h>



K3b::MediumPrivate::MediumPrivate()
    : device( 0 ),
      content( K3b::Medium::CONTENT_NONE )
{
}


K3b::Medium::Medium()
{
    d = new K3b::MediumPrivate;
}



K3b::Medium::Medium( const K3b::Medium& other )
{
    d = other.d;
}


K3b::Medium::Medium( K3b::Device::Device* dev )
{
    d = new K3b::MediumPrivate;
    d->device = dev;
}


K3b::Medium::~Medium()
{
}


K3b::Medium& K3b::Medium::operator=( const K3b::Medium& other )
{
    d = other.d;
    return *this;
}


bool K3b::Medium::isValid() const
{
    return d->device != 0;
}


void K3b::Medium::setDevice( K3b::Device::Device* dev )
{
    if( d->device != dev ) {
        reset();
        d->device = dev;
    }
}

K3b::Device::Device* K3b::Medium::device() const
{
    return d->device;
}


K3b::Device::DiskInfo K3b::Medium::diskInfo() const
{
    return d->diskInfo;
}


K3b::Device::Toc K3b::Medium::toc() const
{
    return d->toc;
}


K3b::Device::CdText K3b::Medium::cdText() const
{
    return d->cdText;
}


KCDDB::CDInfo K3b::Medium::cddbInfo() const
{
    return d->cddbInfo;
}


QList<int> K3b::Medium::writingSpeeds() const
{
    return d->writingSpeeds;
}


K3b::Medium::MediumContents K3b::Medium::content() const
{
    return d->content;
}


const K3b::Iso9660SimplePrimaryDescriptor& K3b::Medium::iso9660Descriptor() const
{
    return d->isoDesc;
}


void K3b::Medium::reset()
{
    d->diskInfo = K3b::Device::DiskInfo();
    d->toc.clear();
    d->cdText.clear();
    d->writingSpeeds.clear();
    d->content = CONTENT_NONE;
    d->cddbInfo.clear();

    // clear the desc
    d->isoDesc = K3b::Iso9660SimplePrimaryDescriptor();
}


void K3b::Medium::update()
{
    if( d->device ) {
        reset();

        d->diskInfo = d->device->diskInfo();

        if( d->diskInfo.diskState() != K3b::Device::STATE_NO_MEDIA ) {
            kDebug() << "(K3b::Medium) found medium: (" << d->device->blockDeviceName() << ')' << endl
                     << "=====================================================";
            d->diskInfo.debug();
            kDebug() << "=====================================================";
        }

        if( diskInfo().diskState() == K3b::Device::STATE_COMPLETE ||
            diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE ) {
            d->toc = d->device->readToc();
            if( d->toc.contentType() == K3b::Device::AUDIO ||
                d->toc.contentType() == K3b::Device::MIXED ) {

                // update CD-Text
                d->cdText = d->device->readCdText();
            }
        }

        if( diskInfo().mediaType() & K3b::Device::MEDIA_WRITABLE ) {
            d->writingSpeeds = d->device->determineSupportedWriteSpeeds();
        }

        analyseContent();
    }
}


void K3b::Medium::analyseContent()
{
    // set basic content types
    switch( d->toc.contentType() ) {
    case K3b::Device::AUDIO:
        d->content = CONTENT_AUDIO;
        break;
    case K3b::Device::DATA:
    case K3b::Device::DVD:
        d->content = CONTENT_DATA;
        break;
    case K3b::Device::MIXED:
        d->content = CONTENT_AUDIO|CONTENT_DATA;
        break;
    default:
        d->content = CONTENT_NONE;
    }

    // analyze filesystem
    if( d->content & CONTENT_DATA ) {
        //kDebug() << "(K3b::Medium) Checking file system.";

        unsigned long startSec = 0;

        if( diskInfo().numSessions() > 1 ) {
            // We use the last data track
            // this way we get the latest session on a ms cd
            K3b::Device::Toc::const_iterator it = d->toc.constEnd();
            --it; // this is valid since there is at least one data track
            while( it != d->toc.constBegin() && (*it).type() != K3b::Device::Track::TYPE_DATA )
                --it;
            startSec = (*it).firstSector().lba();
        }
        else {
            // use first data track
            K3b::Device::Toc::const_iterator it = d->toc.constBegin();
            while( it != d->toc.constEnd() && (*it).type() != K3b::Device::Track::TYPE_DATA )
                ++it;
            startSec = (*it).firstSector().lba();
        }

        //kDebug() << "(K3b::Medium) Checking file system at " << startSec;

        // force the backend since we don't need decryption
        // which just slows down the whole process
        K3b::Iso9660 iso( new K3b::Iso9660DeviceBackend( d->device ) );
        iso.setStartSector( startSec );
        iso.setPlainIso9660( true );
        if( iso.open() ) {
            d->isoDesc = iso.primaryDescriptor();
            kDebug() << "(K3b::Medium) found volume id from start sector " << startSec
                     << ": '" << d->isoDesc.volumeId << "'" ;

            if( diskInfo().isDvdMedia() ) {
                // Every VideoDVD needs to have a VIDEO_TS.IFO file
                if( iso.firstIsoDirEntry()->entry( "VIDEO_TS/VIDEO_TS.IFO" ) != 0 )
                    d->content |= CONTENT_VIDEO_DVD;
            }
            else {
                kDebug() << "(K3b::Medium) checking for VCD.";

                // check for VCD
                const K3b::Iso9660Entry* vcdEntry = iso.firstIsoDirEntry()->entry( "VCD/INFO.VCD" );
                const K3b::Iso9660Entry* svcdEntry = iso.firstIsoDirEntry()->entry( "SVCD/INFO.SVD" );
                const K3b::Iso9660File* vcdInfoFile = 0;
                if( vcdEntry ) {
                    kDebug() << "(K3b::Medium) found vcd entry.";
                    if( vcdEntry->isFile() )
                        vcdInfoFile = static_cast<const K3b::Iso9660File*>(vcdEntry);
                }
                if( svcdEntry && !vcdInfoFile ) {
                    kDebug() << "(K3b::Medium) found svcd entry.";
                    if( svcdEntry->isFile() )
                        vcdInfoFile = static_cast<const K3b::Iso9660File*>(svcdEntry);
                }

                if( vcdInfoFile ) {
                    char buffer[8];

                    if ( vcdInfoFile->read( 0, buffer, 8 ) == 8 &&
                         ( !qstrncmp( buffer, "VIDEO_CD", 8 ) ||
                           !qstrncmp( buffer, "SUPERVCD", 8 ) ||
                           !qstrncmp( buffer, "HQ-VCD  ", 8 ) ) )
                        d->content |= CONTENT_VIDEO_CD;
                }
            }
        }  // opened iso9660
    }
}


QString K3b::Medium::shortString( bool useContent ) const
{
    QString mediaTypeString = K3b::Device::mediaTypeString( diskInfo().mediaType(), true );

    if( diskInfo().diskState() == K3b::Device::STATE_UNKNOWN ) {
        return i18n("No medium information");
    }

    else if( diskInfo().diskState() == K3b::Device::STATE_NO_MEDIA ) {
        return i18n("No medium present");
    }

    else if( diskInfo().diskState() == K3b::Device::STATE_EMPTY ) {
        return i18n("Empty %1 medium", mediaTypeString );
    }

    else {
        if( useContent ) {
            // AUDIO + MIXED
            if( d->toc.contentType() == K3b::Device::AUDIO ||
                d->toc.contentType() == K3b::Device::MIXED ) {
                QString title = cdText().title();
                QString performer = cdText().performer();
                if ( title.isEmpty() ) {
                    title = cddbInfo().get( KCDDB::Title ).toString();
                }
                if ( performer.isEmpty() ) {
                    performer = cddbInfo().get( KCDDB::Artist ).toString();
                }
                if( !performer.isEmpty() && !title.isEmpty() ) {
                    return QString("%1 - %2 (%3)")
                        .arg( performer )
                        .arg( title )
                        .arg( d->toc.contentType() == K3b::Device::AUDIO ? i18n("Audio CD") : i18n("Mixed CD") );
                }
                else if( d->toc.contentType() == K3b::Device::AUDIO ) {
                    return i18n("Audio CD");
                }
                else {
                    return i18n("%1 (Mixed CD)", beautifiedVolumeId() );
                }
            }

            // DATA CD and DVD
            else if( !volumeId().isEmpty() ) {
                if( content() & CONTENT_VIDEO_DVD ) {
                    return QString("%1 (%2)").arg(beautifiedVolumeId()).arg( i18n("Video DVD") );
                }
                else if( content() & CONTENT_VIDEO_CD ) {
                    return QString("%1 (%2)").arg(beautifiedVolumeId()).arg(i18n("Video CD") );
                }
                else if( diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE ) {
                    return i18n("%1 (Appendable Data %2)", beautifiedVolumeId(), mediaTypeString );
                }
                else {
                    return i18n("%1 (Complete Data %2)", beautifiedVolumeId(), mediaTypeString );
                }
            }
            else {
                if( diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE ) {
                    return i18n("Appendable Data %1", mediaTypeString );
                }
                else {
                    return i18n("Complete Data %1", mediaTypeString );
                }
            }
        }

        // without content
        else {
            if( diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE ) {
                return i18n("Appendable %1 medium", mediaTypeString );
            }
            else {
                return i18n("Complete %1 medium", mediaTypeString );
            }
        }
    }
}


QString K3b::Medium::longString() const
{
    QString s = QString("<p><nobr><b>%1 %2</b> (%3)</nobr>"
                        "<p>")
                .arg( d->device->vendor() )
                .arg( d->device->description() )
                .arg( d->device->blockDeviceName() )
                + shortString( true )
                + " (" + K3b::Device::mediaTypeString( diskInfo().mediaType(), true ) + ')';

    if( diskInfo().diskState() == K3b::Device::STATE_COMPLETE ||
        diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE  ) {
        s += "<br>" + i18np("1 in %2 track", "%1 in %2 tracks", d->toc.count(),
                            KIO::convertSize(diskInfo().size().mode1Bytes() ) );
        if( diskInfo().numSessions() > 1 )
            s += i18np(" and %1 session", " and %1 sessions", diskInfo().numSessions() );
    }

    if( diskInfo().diskState() == K3b::Device::STATE_EMPTY ||
        diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE  )
        s += "<br>" + i18n("Free space: %1",
                           KIO::convertSize( diskInfo().remainingSize().mode1Bytes() ) );

    if( !diskInfo().empty() && diskInfo().rewritable() )
        s += "<br>" + i18n("Capacity: %1",
                           KIO::convertSize( diskInfo().capacity().mode1Bytes() ) );

    return s;
}


QString K3b::Medium::volumeId() const
{
    return iso9660Descriptor().volumeId;
}


QString K3b::Medium::beautifiedVolumeId() const
{
    const QString& oldId = volumeId();
    QString newId;

    bool newWord = true;
    for( int i = 0; i < oldId.length(); ++i ) {
        QChar c = oldId[i];
        //
        // first let's handle the cases where we do not change
        // the id anyway
        //
        // In case the id already contains spaces or lower case chars
        // it is likely that it already looks good and does ignore
        // the restricted iso9660 charset (like almost every project
        // created with K3b)
        //
        if( c.isLetter() && c.toLower() == c )
            return oldId;
        else if( c.isSpace() )
            return oldId;

        // replace underscore with space
        else if( c.unicode() == 95 ) {
            newId.append( ' ' );
            newWord = true;
        }

        // from here on only upper case chars and numbers and stuff
        else if( c.isLetter() ) {
            if( newWord ) {
                newId.append( c );
                newWord = false;
            }
            else {
                newId.append( c.toLower() );
            }
        }
        else {
            newId.append( c );
        }
    }

    return newId;
}


KIcon K3b::Medium::icon() const
{
    if ( diskInfo().empty() ) {
        return KIcon( "media-optical-recordable" );
    }
    else if ( content() & CONTENT_AUDIO ) {
        return KIcon( "media-optical-audio" );
    }
    else {
        return KIcon( "media-optical" );
    }
}


bool K3b::Medium::operator==( const K3b::Medium& other ) const
{
    if( this->d == other.d )
        return true;

    return( this->device() == other.device() &&
            this->diskInfo() == other.diskInfo() &&
            this->toc() == other.toc() &&
            this->cdText() == other.cdText() &&
            d->cddbInfo == other.d->cddbInfo &&
            this->content() == other.content() &&
            this->iso9660Descriptor() == other.iso9660Descriptor() );
}


bool K3b::Medium::operator!=( const K3b::Medium& other ) const
{
    if( this->d == other.d )
        return false;

    return( this->device() != other.device() ||
            this->diskInfo() != other.diskInfo() ||
            this->toc() != other.toc() ||
            this->cdText() != other.cdText() ||
            d->cddbInfo != other.d->cddbInfo ||
            this->content() != other.content() ||
            this->iso9660Descriptor() != other.iso9660Descriptor() );
}


bool K3b::Medium::sameMedium( const K3b::Medium& other ) const
{
    if( this->d == other.d )
        return true;

    // here we do ignore cddb info
    return( this->device() == other.device() &&
            this->diskInfo() == other.diskInfo() &&
            this->toc() == other.toc() &&
            this->cdText() == other.cdText() &&
            this->content() == other.content() &&
            this->iso9660Descriptor() == other.iso9660Descriptor() );
}
