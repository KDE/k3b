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

#include "k3bdiskinfodetector.h"

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3bglobals.h>
#include <k3bdevicehandler.h>
#include <k3biso9660.h>

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
      runningHandler(0) {
  }

  CdDevice* device;
  DiskInfo info;
  NextGenerationDiskInfo ngDiskInfo;
  Toc toc;
  AlbumCdText cdText;
  K3bIso9660* iso9660;

  bool isVideoDvd;
  bool isVideoCd;

  K3bCdDevice::DeviceHandler* runningHandler;
};



K3bCdDevice::DiskInfoDetector::DiskInfoDetector( QObject* parent )
  : QObject( parent )
{
  d = new Private();
}


K3bCdDevice::DiskInfoDetector::~DiskInfoDetector()
{
  delete d->iso9660;
  delete d;
}


K3bCdDevice::CdDevice* K3bCdDevice::DiskInfoDetector::device() const
{
  return d->device;
}


// const K3bCdDevice::DiskInfo& K3bCdDevice::DiskInfoDetector::diskInfo() const
// {
//   return d->info;
// }


const K3bCdDevice::NextGenerationDiskInfo& K3bCdDevice::DiskInfoDetector::ngDiskInfo() const
{
  return d->ngDiskInfo;
}


const K3bCdDevice::AlbumCdText& K3bCdDevice::DiskInfoDetector::cdText() const
{
  return d->cdText;
}


const K3bCdDevice::Toc& K3bCdDevice::DiskInfoDetector::toc() const
{
  return d->toc;
}


const K3bIso9660* K3bCdDevice::DiskInfoDetector::iso9660() const
{
  return d->iso9660;
}


void K3bCdDevice::DiskInfoDetector::detect( CdDevice* device )
{
  if( !device ) {
    kdDebug() << "(K3bDiskInfoDetector) detect should really not be called with NULL!" << endl;
    return;
  }

  d->device = device;

  // reset
  delete d->iso9660;
  d->iso9660 = 0;
  //  d->info = DiskInfo();
  //  d->info.device = d->device;

  // we don't want the old info to overrun us...
  if( d->runningHandler )
    d->runningHandler->disconnect( this );

  connect( (d->runningHandler = K3bCdDevice::diskInfo(d->device)),
           SIGNAL(finished(K3bCdDevice::DeviceHandler *)),
           this,
	   SLOT(slotDeviceHandlerFinished(K3bCdDevice::DeviceHandler *)) );
}


void K3bCdDevice::DiskInfoDetector::finish(bool success)
{
  //  d->info.valid=success;
  emit diskInfoReady(this);
}


void K3bCdDevice::DiskInfoDetector::fetchExtraInfo()
{
  kdDebug() << "(K3bCdDevice::DiskInfoDetector) fetchExtraInfo()" << endl;

  d->toc.calculateDiscId();
  //  d->device->indexScan( d->toc );

  bool success = true;

  //
  // FIXME: blank DVD+RW media already has a track so this method tries to open ido9660...
  //

  if( d->toc.contentType() == K3bCdDevice::DATA ||
      d->toc.contentType() == K3bCdDevice::MIXED ) {
    if( d->device->open() != -1 ) {

      unsigned long startSec = 0;

      if( d->ngDiskInfo.numSessions() > 1 ) {
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

      d->iso9660 = new K3bIso9660( d->device, startSec );

      if( d->iso9660->open() ) {
	//	d->iso9660->debug();

// 	d->info.isoId = "CD001";
// 	d->info.isoSystemId = d->iso9660->primaryDescriptor().systemId;
// 	d->info.isoVolumeId = d->iso9660->primaryDescriptor().volumeId;
// 	d->info.isoVolumeSetId = d->iso9660->primaryDescriptor().volumeSetId;
// 	d->info.isoPublisherId = d->iso9660->primaryDescriptor().publisherId;
// 	d->info.isoPreparerId = d->iso9660->primaryDescriptor().preparerId;
// 	d->info.isoApplicationId = d->iso9660->primaryDescriptor().applicationId;
	

	if( K3bCdDevice::isDvdMedia( d->ngDiskInfo.mediaType() ) ) {
	  d->isVideoDvd = false;

	  // We check for the VIDEO_TS directory and at least one .IFO file
	  const K3bIso9660Entry* videoTsEntry = d->iso9660->firstIsoDirEntry()->entry( "VIDEO_TS" );
	  if( videoTsEntry )
	    if( videoTsEntry->isDirectory() ) {
	      kdDebug() << "(K3bDiskInfoDetector) found VIDEO_TS directory." << endl;

	      const K3bIso9660Directory* videoTsDir = static_cast<const K3bIso9660Directory*>( videoTsEntry );
	      QStringList entries = videoTsDir->entries();
	      for( QStringList::const_iterator it = entries.begin(); it != entries.end(); ++it ) {
		if( (*it).right(4) == ".IFO" ) {
		  kdDebug() << "(K3bDiskInfoDetector) found .IFO file: " << *it << endl;
		  d->isVideoDvd = true;
		  break;
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

	d->iso9660->close();
      }  // opened m_iso9660
//       else
// 	success = false;

      d->device->close();
    }  // opened device
    else
      success = false;
  }

  finish(success);
}


void K3bCdDevice::DiskInfoDetector::slotDeviceHandlerFinished( K3bCdDevice::DeviceHandler *handler)
{
  d->runningHandler = 0;

  kdDebug() << "(K3bCdDevice::DiskInfoDetector) slotDeviceHandlerFinished()" << endl;

  bool success = handler->success();
  if( success ) {
    //    d->info = handler->diskInfo();
    d->ngDiskInfo = handler->ngDiskInfo();
    d->ngDiskInfo.debug();
    d->cdText = handler->cdText();
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
