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
 

#include "k3baudioripthread.h"
#include "../device/k3bdevice.h"
#include "../tools/k3bcdparanoialib.h"
#include <k3bjob.h>
#include <k3bprogressinfoevent.h>

#include <qfile.h>
#include <qapplication.h>

#include <kdebug.h>
#include <klocale.h>

// from cdda_paranoia.h
#define PARANOIA_CB_READ           0
#define PARANOIA_CB_VERIFY         1
#define PARANOIA_CB_FIXUP_EDGE     2
#define PARANOIA_CB_FIXUP_ATOM     3
#define PARANOIA_CB_SCRATCH        4
#define PARANOIA_CB_REPAIR         5
#define PARANOIA_CB_SKIP           6
#define PARANOIA_CB_DRIFT          7
#define PARANOIA_CB_BACKOFF        8
#define PARANOIA_CB_OVERLAP        9
#define PARANOIA_CB_FIXUP_DROPPED 10
#define PARANOIA_CB_FIXUP_DUPED   11
#define PARANOIA_CB_READERR       12


static K3bAudioRipThread* s_audioRip = 0;

void paranoiaCallback(long sector, int status)
{
  s_audioRip->createStatus(sector, status);
}



K3bAudioRipThread::K3bAudioRipThread( QObject* parent )
  : QObject( parent ),
    QThread(),
    m_paranoiaLib(0),
    m_device(0),
    m_paranoiaMode(3),
    m_paranoiaRetries(20),
    m_neverSkip(false),
    m_track(1),
    m_eventReceiver(parent)
{
}


K3bAudioRipThread::~K3bAudioRipThread()
{
  delete m_paranoiaLib;
}


void K3bAudioRipThread::start( QObject* eventReceiver )
{
  if( eventReceiver )
    m_eventReceiver = eventReceiver;
  QThread::start();
}



void K3bAudioRipThread::run()
{
  if( !m_paranoiaLib ) {
    m_paranoiaLib = K3bCdparanoiaLib::create();
  }

  if( !m_paranoiaLib ) {
    if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					   new K3bProgressInfoEvent( K3bProgressInfoEvent::InfoMessage,
								     i18n("Could not load libcdparanoia."),
								     QString::null,
								     K3bJob::ERROR ) );
    if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					   new K3bProgressInfoEvent( K3bProgressInfoEvent::Finished,
								     0 ) );
    return;
  }

  // try to open the device
  if( !m_device ) {
    if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					   new K3bProgressInfoEvent( K3bProgressInfoEvent::Finished,
								     0 ) );
    return;
  }

  if( !m_paranoiaLib->paranoiaInit( m_device->blockDeviceName() ) ) {
    if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					   new K3bProgressInfoEvent( K3bProgressInfoEvent::InfoMessage,
								     i18n("Could not open device %1").arg(m_device->blockDeviceName()),
								     QString::null,
								     K3bJob::ERROR ) );
    if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					   new K3bProgressInfoEvent( K3bProgressInfoEvent::Finished,
								     0 ) );
    return;
  }

  m_bInterrupt = m_bError = false;

  long firstSector = m_paranoiaLib->firstSector( m_track );
  m_lastSector = m_paranoiaLib->lastSector( m_track );
  
  if( firstSector < 0 || m_lastSector < 0 ) {
    if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					   new K3bProgressInfoEvent( K3bProgressInfoEvent::Finished,
								     0 ) );
    return;
  }

  kdDebug() << "(K3bAudioRipThread) sectors to read: " << m_lastSector - firstSector << endl;

  // status variable
  m_lastReadSector = 0;
  m_overlap = 0;
  m_readSectors = 0;

  // track length
  m_sectorsAll = m_lastSector - firstSector;
  m_sectorsRead = 0;
  
  m_paranoiaLib->setParanoiaMode( m_paranoiaMode );
  m_paranoiaLib->setNeverSkip( m_neverSkip );
  m_paranoiaLib->setMaxRetries( m_paranoiaRetries );

  m_paranoiaLib->paranoiaSeek( firstSector, SEEK_SET );
  m_currentSector = firstSector;

  while( m_currentSector <= m_lastSector ) {
    if( m_bInterrupt){
      kdDebug() << "(K3bAudioRipThread) Interrupt reading." << endl;
      break;
    } 

    // let the global paranoia callback have access to this
    // to emit signals
    s_audioRip = this;
        
    int16_t* buf = m_paranoiaLib->paranoiaRead( paranoiaCallback );

    if( 0 == buf ) {
      kdDebug() << "(K3bAudioRipThread) Unrecoverable error in paranoia_read" << endl;
      m_bError = true;
      break;
    } 
    else {
      ++m_currentSector;
      QByteArray a;
      a.setRawData( (char*)buf, CD_FRAMESIZE_RAW );
      emit output( a );
      a.resetRawData( (char*)buf, CD_FRAMESIZE_RAW );

      ++m_sectorsRead;
      if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					     new K3bProgressInfoEvent( K3bProgressInfoEvent::Progress,
								       100 * m_sectorsRead / m_sectorsAll ) );	       
    }
  }

  m_paranoiaLib->paranoiaFree();
  if( m_bInterrupt )
    if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					   new K3bProgressInfoEvent( K3bProgressInfoEvent::Canceled ) );

  if( m_bInterrupt || m_bError ) {
    if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					   new K3bProgressInfoEvent( K3bProgressInfoEvent::Finished,
								     0 ) );
  }
  else {
    if( m_eventReceiver ) qApp->postEvent( m_eventReceiver,
					   new K3bProgressInfoEvent( K3bProgressInfoEvent::Finished,
								     1 ) );
  }

}


void K3bAudioRipThread::cancel()
{
  m_bInterrupt = true;
}


void K3bAudioRipThread::createStatus( long sector, int status )
{
  switch( status ) {
  case -1:
    break;
  case -2:
    break;
  case PARANOIA_CB_READ:
    // no problem
    // does only this mean that the sector has been read?
    m_lastReadSector = sector;  // this seems to be rather useless
    m_readSectors++;
    break;
  case PARANOIA_CB_VERIFY:
    break;
  case PARANOIA_CB_FIXUP_EDGE:
    break;
  case PARANOIA_CB_FIXUP_ATOM:
    break;
  case PARANOIA_CB_SCRATCH:
    // scratch detected
    break;
  case PARANOIA_CB_REPAIR:
    break;
  case PARANOIA_CB_SKIP:
    // skipped sector
    break;
  case PARANOIA_CB_DRIFT:
    break;
  case PARANOIA_CB_BACKOFF:
    break;
  case PARANOIA_CB_OVERLAP:
    // sector does not seem to contain the current
    // sector but the amount of overlapped data
    m_overlap = sector;
    break;
  case PARANOIA_CB_FIXUP_DROPPED:
    break;
  case PARANOIA_CB_FIXUP_DUPED:
    break;
  case PARANOIA_CB_READERR:
    break;
  } 
}

#include "k3baudioripthread.moc"

