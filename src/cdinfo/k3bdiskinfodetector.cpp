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

#include <device/k3bdevice.h>
#include <device/k3btoc.h>
#include <k3bglobals.h>
#include <device/k3bdevicehandler.h>
#include <k3biso9660.h>

#include <kdebug.h>

#include <qcstring.h>


class K3bDiskInfoDetector::Private
{
public:
  Private()
    : device(0),
      iso9660(0) {
  }

  CdDevice* device;
  DiskInfo info;
  NextGenerationDiskInfo ngDiskInfo;
  AlbumCdText cdText;
  K3bIso9660* iso9660;
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


const K3bCdDevice::DiskInfo& K3bCdDevice::DiskInfoDetector::diskInfo() const
{
  return d->info;
}


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
  return d->info.toc;
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
  d->info = DiskInfo();
  d->info.device = d->device;
  connect( K3bCdDevice::diskInfo(d->device),
           SIGNAL(finished(K3bCdDevice::DeviceHandler *)),
           this,
	   SLOT(slotDeviceHandlerFinished(K3bCdDevice::DeviceHandler *)) );
}


void K3bCdDevice::DiskInfoDetector::finish(bool success)
{
  d->info.valid=success;
  emit diskInfoReady(this);
}


void K3bCdDevice::DiskInfoDetector::fetchExtraInfo()
{
  kdDebug() << "(K3bCdDevice::DiskInfoDetector) fetchExtraInfo()" << endl;

  d->info.toc.calculateDiscId();

  bool success = true;

  if( d->info.tocType == DiskInfo::DATA ||
      /*      d->info.tocType == DiskInfo::MIXED || */
      d->info.tocType == DiskInfo::DVD ) {
    if( d->device->open() != -1 ) {

      // in mixed mode we seek to the first data track
      // FIXME: this does not work. We need to create a QIODevice for this
      //        which reads via MMC from the medium.
//       if( d->info.tocType == DiskInfo::MIXED ) {
// 	Toc::const_iterator it = toc().begin();
// 	while( it != toc().end() && (*it).type() != Track::DATA )
// 	  ++it;
// 	if( !d->device->seek( (*it).firstSector().lba() ) )
// 	  kdDebug() << "(K3bDiskInfoDetector) seeking to first data track failed." << endl;
//       }

      delete d->iso9660;
      d->iso9660 = new K3bIso9660( d->device->open() );
      if( d->iso9660->open( IO_ReadOnly ) ) {
	d->info.isoId = "CD001";
	d->info.isoSystemId = d->iso9660->primaryDescriptor().systemId;
	d->info.isoVolumeId = d->iso9660->primaryDescriptor().volumeId;
	d->info.isoVolumeSetId = d->iso9660->primaryDescriptor().volumeSetId;
	d->info.isoPublisherId = d->iso9660->primaryDescriptor().publisherId;
	d->info.isoPreparerId = d->iso9660->primaryDescriptor().preparerId;
	d->info.isoApplicationId = d->iso9660->primaryDescriptor().applicationId;
	

	if( d->info.tocType == DiskInfo::DVD ) {
	  d->info.isVideoDvd = false;

	  // We check for the VIDEO_TS directory and at least one .IFO file
	  const KArchiveEntry* videoTsEntry = d->iso9660->directory()->entry( "VIDEO_TS" );
	  if( videoTsEntry )
	    if( videoTsEntry->isDirectory() ) {
	      kdDebug() << "(K3bDiskInfoDetector) found VIDEO_TS directory." << endl;

	      const KArchiveDirectory* videoTsDir = dynamic_cast<const KArchiveDirectory*>( videoTsEntry );
	      QStringList entries = videoTsDir->entries();
	      for( QStringList::const_iterator it = entries.begin(); it != entries.end(); ++it ) {
		if( (*it).right(4) == ".IFO" ) {
		  kdDebug() << "(K3bDiskInfoDetector) found .IFO file: " << *it << endl;
		  d->info.isVideoDvd = true;
		  break;
		}
	      }
	    }
	}
	else {
	  d->info.isVCD = false;

	  kdDebug() << "(K3bDiskInfoDetector) checking for VCD." << endl;

	  // check for VCD
	  const KArchiveEntry* vcdEntry = d->iso9660->directory()->entry( "VCD/INFO.VCD" );
	  const KArchiveEntry* svcdEntry = d->iso9660->directory()->entry( "SVCD/INFO.SVD" );
	  const K3bIso9660File* vcdInfoFile = 0;
	  if( vcdEntry ) {
	    kdDebug() << "(K3bDiskInfoDetector) found vcd entry." << endl;
	    if( vcdEntry->isFile() )
	      vcdInfoFile = (const K3bIso9660File*)vcdEntry;
	  }
	  if( svcdEntry && !vcdInfoFile ) {
	    kdDebug() << "(K3bDiskInfoDetector) found svcd entry." << endl;
	    if( svcdEntry->isFile() )
	      vcdInfoFile = (const K3bIso9660File*)svcdEntry;
	  }

	  if( vcdInfoFile ) {
	    QByteArray buffer = vcdInfoFile->data( 0, 8 );
	    QString info = QString::fromLocal8Bit( buffer.data(), 8 );
	    kdDebug() << "(K3bDiskInfoDetector) VCD: " << info << endl;
	    if ( info == QString("VIDEO_CD") ||
		 info == QString("SUPERVCD") ||
		 info == QString("HQ-VCD  ") )
	      d->info.isVCD = true;
	  }
	}

	d->iso9660->close();
      }  // opened m_iso9660
      else
	success = false;

      d->device->close();
    }  // opened device
    else
      success = false;
  }

  finish(success);
}


void K3bCdDevice::DiskInfoDetector::slotDeviceHandlerFinished( K3bCdDevice::DeviceHandler *handler)
{
  kdDebug() << "(K3bCdDevice::DiskInfoDetector) slotDeviceHandlerFinished()" << endl;

  bool success = handler->success();
  if( success ) {
    d->info = handler->diskInfo();
    d->ngDiskInfo = handler->ngDiskInfo();
    d->ngDiskInfo.debug();
    d->cdText = handler->cdText();
    fetchExtraInfo();
  }
  else
    finish( false );
}

#include "k3bdiskinfodetector.moc"
