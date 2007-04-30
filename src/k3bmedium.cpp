
/*
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include <config.h>

#include "k3bmedium.h"

#include <k3bdeviceglobals.h>
#include <k3bglobals.h>
#include <k3biso9660.h>
#include <k3biso9660backend.h>

#include <klocale.h>
#include <kio/global.h>

/**
 * Internal class used by K3bMedium
 */
class K3bMedium::Data : public KShared
{
public:
  Data();

  K3bDevice::Device* device;
  K3bDevice::DiskInfo diskInfo;
  K3bDevice::Toc toc;
  K3bDevice::CdText cdText;
  QValueList<int> writingSpeeds;
  K3bIso9660SimplePrimaryDescriptor isoDesc;
  int content;
};



K3bMedium::Data::Data()
  : device( 0 ),
    content( K3bMedium::CONTENT_NONE )
{
}


K3bMedium::K3bMedium()
{
  d = new Data;
}



K3bMedium::K3bMedium( const K3bMedium& other )
{
  d = other.d;
}


K3bMedium::K3bMedium( K3bDevice::Device* dev )
{
  d = new Data;
  d->device = dev;
}


K3bMedium::~K3bMedium()
{
}


K3bMedium& K3bMedium::operator=( const K3bMedium& other )
{
  if( this != &other )
    d = other.d;
  return *this;
}


void K3bMedium::detach()
{
  if( d.count() > 1 )
    d = new Data( *d );
}


void K3bMedium::setDevice( K3bDevice::Device* dev )
{
  if( d->device != dev ) {
    reset();
    d->device = dev;
  }
}

K3bDevice::Device* K3bMedium::device() const
{
  return d->device;
}


const K3bDevice::DiskInfo& K3bMedium::diskInfo() const
{
  return d->diskInfo;
}


const K3bDevice::Toc& K3bMedium::toc() const
{
  return d->toc;
}


const K3bDevice::CdText& K3bMedium::cdText() const
{
  return d->cdText;
}


const QValueList<int>& K3bMedium::writingSpeeds() const
{
  return d->writingSpeeds;
}


int K3bMedium::content() const
{
  return d->content;
}


const K3bIso9660SimplePrimaryDescriptor& K3bMedium::iso9660Descriptor() const
{
  return d->isoDesc;
}


void K3bMedium::reset()
{
  detach();

  d->diskInfo = K3bDevice::DiskInfo();
  d->toc.clear();
  d->cdText.clear();
  d->writingSpeeds.clear();
  d->content = CONTENT_NONE;

  // clear the desc
  d->isoDesc = K3bIso9660SimplePrimaryDescriptor();
}


void K3bMedium::update()
{
  if( d->device ) {
    reset();

    d->diskInfo = d->device->diskInfo();

    if( d->diskInfo.diskState() != K3bDevice::STATE_NO_MEDIA ) {
      kdDebug() << "(K3bMedium) found medium:" << endl
		<< "=====================================================" << endl;
      d->diskInfo.debug();
      kdDebug() << "=====================================================" << endl;
    }

    if( diskInfo().diskState() == K3bDevice::STATE_COMPLETE ||
	diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE ) {
      d->toc = d->device->readToc();
      if( d->toc.contentType() == K3bDevice::AUDIO ||
	  d->toc.contentType() == K3bDevice::MIXED )
	d->cdText = d->device->readCdText();
    }

    if( diskInfo().mediaType() & K3bDevice::MEDIA_WRITABLE ) {
      d->writingSpeeds = d->device->determineSupportedWriteSpeeds();
    }

    analyseContent();
  }
}


void K3bMedium::analyseContent()
{
  // set basic content types
  switch( toc().contentType() ) {
    case K3bDevice::AUDIO:
      d->content = CONTENT_AUDIO;
      break;
    case K3bDevice::DATA:
    case K3bDevice::DVD:
      d->content = CONTENT_DATA;
      break;
    case K3bDevice::MIXED:
      d->content = CONTENT_AUDIO|CONTENT_DATA;
      break;
    default:
      d->content = CONTENT_NONE;
  }

  // analyze filesystem
  if( d->content & CONTENT_DATA ) {
    //kdDebug() << "(K3bMedium) Checking file system." << endl;

    unsigned long startSec = 0;

    if( diskInfo().numSessions() > 1 ) {
      // We use the last data track
      // this way we get the latest session on a ms cd
      K3bDevice::Toc::const_iterator it = toc().end();
      --it; // this is valid since there is at least one data track
      while( it != toc().begin() && (*it).type() != K3bDevice::Track::DATA )
	--it;
      startSec = (*it).firstSector().lba();
    }
    else {
      // use first data track
      K3bDevice::Toc::const_iterator it = toc().begin();
      while( it != toc().end() && (*it).type() != K3bDevice::Track::DATA )
	++it;
      startSec = (*it).firstSector().lba();
    }

    //kdDebug() << "(K3bMedium) Checking file system at " << startSec << endl;

    // force the backend since we don't need decryption
    // which just slows down the whole process
    K3bIso9660 iso( new K3bIso9660DeviceBackend( d->device ) );
    iso.setStartSector( startSec );
    iso.setPlainIso9660( true );
    if( iso.open() ) {
      d->isoDesc = iso.primaryDescriptor();
      kdDebug() << "(K3bMedium) found volume id from start sector " << startSec
		<< ": '" << d->isoDesc.volumeId << "'" << endl;

      if( diskInfo().isDvdMedia() ) {
	// Every VideoDVD needs to have a VIDEO_TS.IFO file
	if( iso.firstIsoDirEntry()->entry( "VIDEO_TS/VIDEO_TS.IFO" ) != 0 )
	  d->content |= CONTENT_VIDEO_DVD;
      }
      else {
	kdDebug() << "(K3bMedium) checking for VCD." << endl;

	// check for VCD
	const K3bIso9660Entry* vcdEntry = iso.firstIsoDirEntry()->entry( "VCD/INFO.VCD" );
	const K3bIso9660Entry* svcdEntry = iso.firstIsoDirEntry()->entry( "SVCD/INFO.SVD" );
	const K3bIso9660File* vcdInfoFile = 0;
	if( vcdEntry ) {
	  kdDebug() << "(K3bMedium) found vcd entry." << endl;
	  if( vcdEntry->isFile() )
	    vcdInfoFile = static_cast<const K3bIso9660File*>(vcdEntry);
	}
	if( svcdEntry && !vcdInfoFile ) {
	  kdDebug() << "(K3bMedium) found svcd entry." << endl;
	  if( svcdEntry->isFile() )
	    vcdInfoFile = static_cast<const K3bIso9660File*>(svcdEntry);
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


QString K3bMedium::shortString( bool useContent ) const
{
  QString mediaTypeString = K3bDevice::mediaTypeString( diskInfo().mediaType(), true );

  if( diskInfo().diskState() == K3bDevice::STATE_UNKNOWN ) {
    return i18n("No medium information");
  }

  else if( diskInfo().diskState() == K3bDevice::STATE_NO_MEDIA ) {
    return i18n("No medium present");
  }

  else if( diskInfo().diskState() == K3bDevice::STATE_EMPTY ) {
    return i18n("Empty %1 medium").arg( mediaTypeString );
  }

  else {
    if( useContent ) {
      // AUDIO + MIXED
      if( toc().contentType() == K3bDevice::AUDIO ||
	  toc().contentType() == K3bDevice::MIXED ) {
	if( !cdText().performer().isEmpty() || !cdText().title().isEmpty() ) {
	  return QString("%1 - %2 (%3)")
	    .arg( cdText().performer() )
	    .arg( cdText().title() )
	    .arg( toc().contentType() == K3bDevice::AUDIO ? i18n("Audio CD") : i18n("Mixed CD") );
	}
	else if( toc().contentType() == K3bDevice::AUDIO ) {
	  return i18n("Audio CD");
	}
	else {
	  return i18n("%1 (Mixed CD)").arg( beautifiedVolumeId() );
	}
      }

      // DATA CD and DVD
      else if( !volumeId().isEmpty() ) {
	if( content() & CONTENT_VIDEO_DVD ) {
	  return QString("%1 (%2)").arg( beautifiedVolumeId() ).arg( i18n("Video DVD") );
	}
	else if( content() & CONTENT_VIDEO_CD ) {
	  return QString("%1 (%2)").arg( beautifiedVolumeId() ).arg( i18n("Video CD") );
	}
	else if( diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE ) {
	  return i18n("%1 (Appendable Data %2)").arg( beautifiedVolumeId(), mediaTypeString );
	}
	else {
	  return i18n("%1 (Complete Data %2)").arg( beautifiedVolumeId(), mediaTypeString );
	}
      }
      else {
	if( diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE ) {
	  return i18n("Appendable Data %1").arg( mediaTypeString );
	}
	else {
	  return i18n("Complete Data %1").arg( mediaTypeString );
	}
      }
    }

    // without content
    else {
      if( diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE ) {
	return i18n("Appendable %1 medium").arg( mediaTypeString );
      }
      else {
	return i18n("Complete %1 medium").arg( mediaTypeString );
      }
    }
  }
}


QString K3bMedium::longString() const
{
  QString s = QString("<p><nobr><b>%1 %2</b> (%3)</nobr>"
		      "<p>")
    .arg( d->device->vendor() )
    .arg( d->device->description() )
    .arg( d->device->blockDeviceName() )
    + shortString( true );

  if( diskInfo().diskState() == K3bDevice::STATE_COMPLETE ||
      diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE  ) {
    s += "<br>" + i18n("%1 in %n track", "%1 in %n tracks", toc().count() )
      .arg( KIO::convertSize(diskInfo().size().mode1Bytes() ) );
    if( diskInfo().numSessions() > 1 )
      s += i18n(" and %n session", " and %n sessions", diskInfo().numSessions() );
  }

  if( diskInfo().diskState() == K3bDevice::STATE_EMPTY ||
      diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE  )
    s += "<br>" + i18n("Free space: %1")
      .arg( KIO::convertSize( diskInfo().remainingSize().mode1Bytes() ) );

  if( !diskInfo().empty() && diskInfo().rewritable() )
    s += "<br>" + i18n("Capacity: %1")
      .arg( KIO::convertSize( diskInfo().capacity().mode1Bytes() ) );

  return s;
}


const QString& K3bMedium::volumeId() const
{
  return iso9660Descriptor().volumeId;
}


QString K3bMedium::beautifiedVolumeId() const
{
  const QString& oldId = volumeId();
  QString newId;

  bool newWord = true;
  for( unsigned int i = 0; i < oldId.length(); ++i ) {
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
    if( c.isLetter() && c.lower() == c )
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
	newId.append( c.lower() );
      }
    }
    else {
      newId.append( c );
    }
  }

  return newId;
}


bool K3bMedium::operator==( const K3bMedium& other )
{
  if( this->d == other.d )
    return true;

  return( this->device() == other.device() &&
	  this->diskInfo() == other.diskInfo() &&
	  this->toc() == other.toc() &&
	  this->cdText() == other.cdText() &&
	  this->content() == other.content() &&
	  this->iso9660Descriptor() == other.iso9660Descriptor() );
}


bool K3bMedium::operator!=( const K3bMedium& other )
{
  if( this->d == other.d )
    return false;

  return( this->device() != other.device() ||
	  this->diskInfo() != other.diskInfo() ||
	  this->toc() != other.toc() ||
	  this->cdText() != other.cdText() ||
	  this->content() != other.content() ||
	  this->iso9660Descriptor() != other.iso9660Descriptor() );
}
