
/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
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



K3bMedium::K3bMedium()
  : m_device( 0 ),
    m_isoDesc( 0 ),
    m_content( CONTENT_NONE )
{
}


K3bMedium::K3bMedium( K3bDevice::Device* dev )
  : m_device( dev ),
    m_isoDesc( 0 ),
    m_content( CONTENT_NONE )
{
}


K3bMedium::~K3bMedium()
{
}


void K3bMedium::reset()
{
  m_diskInfo = K3bDevice::DiskInfo();
  m_toc.clear();
  m_cdText.clear();
  m_writingSpeeds.clear();
  delete m_isoDesc;
  m_isoDesc = 0;
  m_content = CONTENT_NONE;
}


void K3bMedium::update()
{
  if( m_device ) {
    reset();

    m_diskInfo = m_device->diskInfo();
    
    if( diskInfo().diskState() == K3bDevice::STATE_COMPLETE || 
	diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE ) {
      m_toc = m_device->readToc();
      if( m_toc.contentType() == K3bDevice::AUDIO ||
	  m_toc.contentType() == K3bDevice::MIXED )
	m_cdText = m_device->readCdText();
    }
    
    if( diskInfo().mediaType() & K3bDevice::MEDIA_WRITABLE ) {
      m_writingSpeeds = m_device->determineSupportedWriteSpeeds();

      // some older drives do not report the speeds properly
      if( m_writingSpeeds.isEmpty() ) {
	// add speeds up to the max
	int max = m_device->determineMaximalWriteSpeed();
	int i = 1;
	int speed = ( m_diskInfo.isDvdMedia() ? 1385 : 175 );
	while( i*speed <= max ) {
	  m_writingSpeeds.append( i*speed );
	  i = ( i == 1 ? 2 : i+2 );
	}
      }
    }

    analyseContent();

//       unsigned char data[2048];
//       if( m_device->read10( data, 2048, startSec+16, 1 ) ) {
// 	m_isoDesc->systemId = QCString( (char*)&data[8], 32 ).stripWhiteSpace();
// 	m_isoDesc->volumeId = QCString( (char*)&data[40], 32 ).stripWhiteSpace();
// 	m_isoDesc->applicationId = QCString( (char*)&data[574], 128 ).stripWhiteSpace();
// 	m_isoDesc->publisherId = QCString( (char*)&data[318], 128 ).stripWhiteSpace();
// 	m_isoDesc->preparerId = QCString( (char*)&data[446], 128 ).stripWhiteSpace();
// 	m_isoDesc->volumeSetId = QCString( (char*)&data[190], 128 ).stripWhiteSpace();
// 	m_isoDesc->volumeSetSize = K3b::fromLe32( (char*)&data[120] );
// 	m_isoDesc->volumeSetNumber = K3b::fromLe32( (char*)&data[124] );
// 	m_isoDesc->logicalBlockSize = K3b::fromLe32( (char*)&data[128] );
// 	m_isoDesc->volumeSpaceSize = K3b::fromLe64( (char*)&data[80] );
// 
// 	kdDebug() << "(K3bMedium) found volume id from start sector " << startSec 
// 		  << ": '" << m_isoDesc->volumeId << "'" << endl;
  }
}


void K3bMedium::analyseContent()
{
  // set basic content types
  switch( toc().contentType() ) {
    case K3bDevice::AUDIO:
      m_content = CONTENT_AUDIO;
      break;
    case K3bDevice::DATA:
    case K3bDevice::DVD:
      m_content = CONTENT_DATA;
      break;
    case K3bDevice::MIXED:
      m_content = CONTENT_AUDIO|CONTENT_DATA;
      break;
    default:
      m_content = CONTENT_NONE;
  }

  // analyse filesystem
  if( m_content & CONTENT_DATA ) {
    //kdDebug() << "(K3bMedium) Checking file system." << endl;
    if( !m_isoDesc )
      m_isoDesc = new K3bIso9660SimplePrimaryDescriptor;

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
      K3bIso9660 iso( new K3bIso9660DeviceBackend( m_device ) );
      iso.setStartSector( startSec );
      iso.setPlainIso9660( true );
      if( iso.open() ) {
	*m_isoDesc = iso.primaryDescriptor();
 	kdDebug() << "(K3bMedium) found volume id from start sector " << startSec 
 		  << ": '" << m_isoDesc->volumeId << "'" << endl;

	if( diskInfo().isDvdMedia() ) {
	  // Every VideoDVD needs to have a VIDEO_TS.IFO file
	  if( iso.firstIsoDirEntry()->entry( "VIDEO_TS/VIDEO_TS.IFO" ) != 0 )
	    m_content |= CONTENT_VIDEO_DVD;
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
	      m_content |= CONTENT_VIDEO_CD;
	  }
	}
      }  // opened iso9660
  }
  else {
    delete m_isoDesc;
    m_isoDesc = 0;
  }
}


QString K3bMedium::shortString( bool useContent ) const
{
  QString mediaTypeString = K3bDevice::mediaTypeString( diskInfo().mediaType(), true );
  
  if( diskInfo().diskState() == K3bDevice::STATE_UNKNOWN ) {
    return i18n("No medium information...");
  }

  else if( diskInfo().diskState() == K3bDevice::STATE_NO_MEDIA ) {
    return i18n("No medium");
  }

  else if( diskInfo().diskState() == K3bDevice::STATE_EMPTY ) {
    return i18n("Empty %1 medium").arg( mediaTypeString );
  }

  else {
    if( useContent ) {
      // AUDIO + MIXED
      if( toc().contentType() == K3bDevice::AUDIO ||
	  toc().contentType() == K3bDevice::MIXED ) {
	if( !cdText().isEmpty() )
	  return QString("%1 - %2 (%3)")
	    .arg( cdText().performer() )
	    .arg( cdText().title() )
	    .arg( toc().contentType() == K3bDevice::AUDIO ? i18n("Audio CD") : i18n("Mixed CD") );
	else if( toc().contentType() == K3bDevice::AUDIO )
	  return i18n("Audio CD");
	else
	  return i18n("%1 (mixed mode CD)").arg( volumeId() );
      }
      
      // DATA CD and DVD
      else if( !volumeId().isEmpty() ) {
	if( diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE )
	  return i18n("%1 (appendable data %2)").arg( volumeId(), mediaTypeString );
	else
	  return i18n("%1 (full data %2)").arg( volumeId(), mediaTypeString );
      }
      else {
	if( diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE )
	  return i18n("Appendable data %1").arg( mediaTypeString );
	else
	  return i18n("Full data %1").arg( mediaTypeString );
      }
    }
    
    // without content
    else {
      if( diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE )
	return i18n("Appendable %1 medium").arg( mediaTypeString );
      else
	return i18n("Full %1 medium").arg( mediaTypeString );
    }
  }
}


QString K3bMedium::longString() const
{
  QString s = QString("<p><nobr><b>%1 %2</b> (%3)</nobr>"
		      "<p>")
    .arg( m_device->vendor() )
    .arg( m_device->description() )
    .arg( m_device->blockDeviceName() )
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
  if( iso9660Descriptor() )
    return iso9660Descriptor()->volumeId;
  else {
    static QString dummy;
    return dummy;
  }
}
