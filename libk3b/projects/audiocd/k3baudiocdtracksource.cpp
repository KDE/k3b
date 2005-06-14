/* 
 *
 * $Id$
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

#include "k3baudiocdtracksource.h"
#include "k3baudiotrack.h"
#include "k3baudiodoc.h"

#include <k3bthreadwidget.h>
#include <k3btoc.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bcdparanoialib.h>
#include <k3bdeviceselectiondialog.h>
#include <k3bcore.h>

#include <klocale.h>
#include <kdebug.h>


K3bAudioCdTrackSource::K3bAudioCdTrackSource( const K3bDevice::Toc& toc, int cdTrackNumber, 
					      const K3bCddbResultEntry& cddb, K3bDevice::Device* dev )
  : K3bAudioDataSource(),
    m_toc( toc ),
    m_cdTrackNumber( cdTrackNumber ),
    m_cddbEntry( cddb ),
    m_lastUsedDevice( dev ),
    m_cdParanoiaLib( 0 ),
    m_initialized( false )
{
}


K3bAudioCdTrackSource::K3bAudioCdTrackSource( const K3bAudioCdTrackSource& source )
  : K3bAudioDataSource( source ),
    m_toc( source.m_toc ),
    m_cdTrackNumber( source.m_cdTrackNumber ),
    m_cddbEntry( source.m_cddbEntry ),
    m_lastUsedDevice( source.m_lastUsedDevice ),
    m_cdParanoiaLib( 0 ),
    m_initialized( false )
{
}


K3bAudioCdTrackSource::~K3bAudioCdTrackSource()
{
  delete m_cdParanoiaLib;
}


bool K3bAudioCdTrackSource::initParanoia()
{
  if( !m_initialized ) {
    if( !m_cdParanoiaLib )
      m_cdParanoiaLib = K3bCdparanoiaLib::create();
    
    if( m_cdParanoiaLib ) {
      m_lastUsedDevice = searchForAudioCD();

      // ask here for the cd since searchForAudioCD() may also be called from outside
      if( !m_lastUsedDevice ) {
	// could not find the CD, so ask for it
	QString s = i18n("Please insert Audio CD %1%2")
	  .arg(m_toc.discId(), 0, 16)
	  .arg(m_cddbEntry.cdTitle.isEmpty() || m_cddbEntry.cdArtist.isEmpty() 
	       ? QString::null 
	       : " (" + m_cddbEntry.cdArtist + " - " + m_cddbEntry.cdTitle + ")");
	
	while( K3bDevice::Device* dev = K3bThreadWidget::selectDevice( track()->doc()->view(), s ) ) {
	  if( searchForAudioCD( dev ) ) {
	    m_lastUsedDevice = dev;
	    break;
	  }
	}
      }

      // user canceled
      if( !m_lastUsedDevice )
	return false;

      if( !m_cdParanoiaLib->initParanoia( m_lastUsedDevice, m_toc ) )
	return false;

      if( doc() ) {
	m_cdParanoiaLib->setParanoiaMode( doc()->audioRippingParanoiaMode() );
	m_cdParanoiaLib->setNeverSkip( !doc()->audioRippingIgnoreReadErrors() );
	m_cdParanoiaLib->setMaxRetries( doc()->audioRippingRetries() );
      }

      m_cdParanoiaLib->initReading( m_toc[m_cdTrackNumber-1].firstSector().lba() + startOffset().lba() + m_position.lba(), 
				    m_toc[m_cdTrackNumber-1].firstSector().lba() + lastSector().lba() );

      m_initialized = true;
      kdDebug() << "(K3bAudioCdTrackSource) initialized." << endl;
    }
  }

  return m_initialized;
}


void K3bAudioCdTrackSource::closeParanoia()
{
  if( m_cdParanoiaLib && m_initialized )
    m_cdParanoiaLib->close();
  m_initialized = false;
}


K3bDevice::Device* K3bAudioCdTrackSource::searchForAudioCD() const
{
  kdDebug() << "(K3bAudioCdTrackSource::searchForAudioCD()" << endl;
  // first try the saved device
  if( m_lastUsedDevice && searchForAudioCD( m_lastUsedDevice ) )
    return m_lastUsedDevice;

  QPtrList<K3bDevice::Device>& devices = k3bcore->deviceManager()->readingDevices();
  for( QPtrListIterator<K3bDevice::Device> it(devices); *it; ++it ) {
    if( searchForAudioCD( *it ) ) {
      return *it;
    }
  }

  kdDebug() << "(K3bAudioCdTrackSource::searchForAudioCD) failed." << endl;

  return 0;
}


bool K3bAudioCdTrackSource::searchForAudioCD( K3bDevice::Device* dev ) const
{
  kdDebug() << "(K3bAudioCdTrackSource::searchForAudioCD(" << dev->description() << ")" << endl;
  K3bDevice::Toc toc = dev->readToc();
  return ( toc.discId() == m_toc.discId() );
}


void K3bAudioCdTrackSource::setDevice( K3bDevice::Device* dev )
{
  if( dev && dev != m_lastUsedDevice ) {
    m_lastUsedDevice = dev;
    if( m_initialized ) {
    }
  }
}


K3b::Msf K3bAudioCdTrackSource::originalLength() const
{
  return m_toc[m_cdTrackNumber-1].length();
}


bool K3bAudioCdTrackSource::seek( const K3b::Msf& msf )
{
  // HACK: to reinitialize everytime we restart the decoding
  if( msf == 0 && m_cdParanoiaLib )
    closeParanoia();

  m_position = msf;

  if( m_cdParanoiaLib )
    m_cdParanoiaLib->initReading( m_toc[m_cdTrackNumber-1].firstSector().lba() + startOffset().lba() + m_position.lba(), 
				  m_toc[m_cdTrackNumber-1].firstSector().lba() + lastSector().lba() );

  return true;
}


int K3bAudioCdTrackSource::read( char* data, unsigned int )
{
  if( initParanoia() ) {
    int status = 0;
    char* buf = m_cdParanoiaLib->read( &status, 0, false /* big endian */ );
    if( status == K3bCdparanoiaLib::S_OK ) {
      if( buf == 0 ) {
	// done
	closeParanoia();
	return 0;
      }
      else {
	++m_position;
	::memcpy( data, buf, CD_FRAMESIZE_RAW );
	return CD_FRAMESIZE_RAW;
      }
    }
    else {
      // in case the reading fails we go back to "not initialized"
      closeParanoia();
      return -1;
    }
  }
  else
    return -1;
}


QString K3bAudioCdTrackSource::type() const
{
  return i18n("CD Track");
}


QString K3bAudioCdTrackSource::sourceComment() const
{
  return i18n("Track %1 from Audio CD %2").arg(m_cdTrackNumber).arg(m_toc.discId(),0,16);
}


K3bAudioDataSource* K3bAudioCdTrackSource::copy() const
{
  return new K3bAudioCdTrackSource( *this );
}
