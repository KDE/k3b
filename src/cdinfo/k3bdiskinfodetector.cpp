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

#include "k3bdiskinfodetector.h"
#include "../k3bmediacache.h"
#include "../k3bmedium.h"
#include "../k3bapplication.h"

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3bglobals.h>
#include <k3bdevicehandler.h>
#include <k3biso9660.h>
#include <k3biso9660backend.h>

#include <kdebug.h>

#include <qcstring.h>


class K3bDiskInfoDetector::Private
{
public:
  Private()
    : device(0),
      iso9660(0),
      isVideoDvd(false),
      isVideoCd(false),
      deviceHandler(0) {
  }

  Device* device;
  DiskInfo diskInfo;
  Toc toc;
  CdText cdText;
  K3bIso9660* iso9660;

  bool isVideoDvd;
  bool isVideoCd;

  K3bDevice::DeviceHandler* deviceHandler;
};



K3bDevice::DiskInfoDetector::DiskInfoDetector( QObject* parent )
  : QObject( parent )
{
  d = new Private();

  d->deviceHandler = new K3bDevice::DeviceHandler( this );

  connect( d->deviceHandler,
           SIGNAL(finished(K3bDevice::DeviceHandler *)),
           this,
	   SLOT(slotDeviceHandlerFinished(K3bDevice::DeviceHandler *)) );
}


K3bDevice::DiskInfoDetector::~DiskInfoDetector()
{
  delete d->iso9660;
  delete d;
}


K3bDevice::Device* K3bDevice::DiskInfoDetector::device() const
{
  return d->device;
}


const K3bDevice::DiskInfo& K3bDevice::DiskInfoDetector::diskInfo() const
{
  return d->diskInfo;
}


const K3bDevice::CdText& K3bDevice::DiskInfoDetector::cdText() const
{
  return d->cdText;
}


const K3bDevice::Toc& K3bDevice::DiskInfoDetector::toc() const
{
  return d->toc;
}


const K3bIso9660* K3bDevice::DiskInfoDetector::iso9660() const
{
  return d->iso9660;
}


void K3bDevice::DiskInfoDetector::detect( Device* device )
{
  if( !device ) {
    kdDebug() << "(K3bDiskInfoDetector) detect should really not be called with NULL!" << endl;
    return;
  }

  d->device = device;
  d->isVideoCd = false;
  d->isVideoDvd = false;

  // reset
  delete d->iso9660;
  d->iso9660 = 0;

  d->deviceHandler->setDevice( device );
  d->deviceHandler->sendCommand( K3bDevice::DeviceHandler::DISKINFO );
}


void K3bDevice::DiskInfoDetector::finish(bool success)
{
  emit diskInfoReady(this);

  // close the iso stuff after informing the peers
  if( d->iso9660 )
    d->iso9660->close();
}


void K3bDevice::DiskInfoDetector::fetchExtraInfo()
{
  if( d->toc.contentType() == K3bDevice::DATA ||
      d->toc.contentType() == K3bDevice::MIXED ) {
    if( d->device->open() ) {

      unsigned long startSec = 0;

      if( d->diskInfo.numSessions() > 1 ) {
	// We use the last data track
	// this way we get the latest session on a ms cd
	Toc::const_iterator it = toc().end();
	--it; // this is valid since there is at least one data track
	while( it != toc().begin() && (*it).type() != Track::DATA )
	  --it;
	startSec = (*it).firstSector().lba();
      }
      else {
	// use first data track
	Toc::const_iterator it = toc().begin();
	while( it != toc().end() && (*it).type() != Track::DATA )
	  ++it;
	startSec = (*it).firstSector().lba();
      }

      delete d->iso9660;

      // force the backend since for the descriptors we don't need decryption
      // which just slows down the whole thing
      d->iso9660 = new K3bIso9660( new K3bIso9660DeviceBackend( d->device ) );
      d->iso9660->setStartSector( startSec );

      if( d->iso9660->open() ) {
	if( K3bDevice::isDvdMedia( d->diskInfo.mediaType() ) ) {
	  d->isVideoDvd = false;

	  // We check for the VIDEO_TS directory and at least one .IFO file
	  const K3bIso9660Entry* videoTsEntry = d->iso9660->firstIsoDirEntry()->entry( "VIDEO_TS" );
	  if( videoTsEntry ) {
	    if( videoTsEntry->isDirectory() ) {
	      kdDebug() << "(K3bDiskInfoDetector) found VIDEO_TS directory." << endl;

	      const K3bIso9660Directory* videoTsDir = static_cast<const K3bIso9660Directory*>( videoTsEntry );
	      QStringList entries = videoTsDir->iso9660Entries();
	      for( QStringList::const_iterator it = entries.begin(); it != entries.end(); ++it ) {
		if( (*it).right(4) == ".IFO" ) {
		  kdDebug() << "(K3bDiskInfoDetector) found .IFO file: " << *it << endl;
		  d->isVideoDvd = true;
		  break;
		}
	      }
	    }
	  }
	}
	else {
	  d->isVideoCd = false;

	  kdDebug() << "(K3bDiskInfoDetector) checking for VCD." << endl;

	  // check for VCD
	  const K3bIso9660Entry* vcdEntry = d->iso9660->firstIsoDirEntry()->entry( "VCD/INFO.VCD" );
	  const K3bIso9660Entry* svcdEntry = d->iso9660->firstIsoDirEntry()->entry( "SVCD/INFO.SVD" );
	  const K3bIso9660File* vcdInfoFile = 0;
	  if( vcdEntry ) {
	    kdDebug() << "(K3bDiskInfoDetector) found vcd entry." << endl;
	    if( vcdEntry->isFile() )
	      vcdInfoFile = static_cast<const K3bIso9660File*>(vcdEntry);
	  }
	  if( svcdEntry && !vcdInfoFile ) {
	    kdDebug() << "(K3bDiskInfoDetector) found svcd entry." << endl;
	    if( svcdEntry->isFile() )
	      vcdInfoFile = static_cast<const K3bIso9660File*>(svcdEntry);
	  }

	  if( vcdInfoFile ) {
	    char buffer[8];
	    vcdInfoFile->read( 0, buffer, 8 );
	    QString info = QString::fromLocal8Bit( buffer, 8 );
	    kdDebug() << "(K3bDiskInfoDetector) VCD: " << info << endl;
	    if ( info == QString("VIDEO_CD") ||
		 info == QString("SUPERVCD") ||
		 info == QString("HQ-VCD  ") )
	      d->isVideoCd = true;
	  }
	}
      }  // opened m_iso9660

      d->device->close();
    }  // opened device
  }

  finish(true);
}


void K3bDevice::DiskInfoDetector::slotDeviceHandlerFinished( K3bDevice::DeviceHandler* handler )
{
  kdDebug() << "(K3bDevice::DiskInfoDetector) slotDeviceHandlerFinished()" << endl;

  bool success = handler->success();
  if( success ) {
    //    d->info = handler->diskInfo();
    d->diskInfo = handler->diskInfo();
    d->diskInfo.debug();
    d->cdText = handler->cdText();

    // some debugging
    CdText x( d->cdText.rawPackData() );

    d->toc = handler->toc();
    fetchExtraInfo();
  }
  else
    finish( false );
}


bool K3bDiskInfoDetector::isVideoDvd() const
{
  return d->isVideoDvd;
}


bool K3bDiskInfoDetector::isVideoCd() const
{
  return d->isVideoCd;
}

#include "k3bdiskinfodetector.moc"
