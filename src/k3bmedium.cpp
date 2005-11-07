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

#include "k3bmedium.h"

#include <k3bdeviceglobals.h>

#include <klocale.h>
#include <kio/global.h>



K3bMedium::K3bMedium()
  : m_device( 0 )
{
}


K3bMedium::K3bMedium( K3bDevice::Device* dev )
  : m_device( dev )
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
  m_volumeId.truncate(0);
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

    if( !toc().isEmpty() && toc().contentType() != K3bDevice::AUDIO ) {
      // determine volume id
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

      unsigned char data[2048];
      if( m_device->read10( data, 2048, startSec+16, 1 ) ) {
	QCString s( (char*)&data[40], 33 );
	m_volumeId = s.stripWhiteSpace();
	kdDebug() << "(K3bMedium) found volume id from start sector " << startSec << ": '" << s << "'" << endl;
      }
    }
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
      else {
	if( diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE )
	  return i18n("%1 (appendable data %2)").arg( volumeId(), mediaTypeString );
	else
	  return i18n("%1 (full data %2)").arg( volumeId(), mediaTypeString );
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
