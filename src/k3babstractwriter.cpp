/***************************************************************************
                          k3babstractwriter.cpp  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3babstractwriter.h"

#include <k3bdevicemanager.h>


K3bAbstractWriter::K3bAbstractWriter( K3bDevice* dev, QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_burnDevice(dev),
    m_burnSpeed(1),
    m_burnproof(false),
    m_simulate(false),
    m_started(false)
{
  connect( this, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)) );
  connect( this, SIGNAL(processedSize(int, int)), this, SLOT(slotProcessedSize(int, int)) );
}


K3bAbstractWriter::~K3bAbstractWriter()
{
}


K3bDevice* K3bAbstractWriter::burnDevice() const
{
  if( m_burnDevice )
    return m_burnDevice; 
  else
    return K3bDeviceManager::self()->burningDevices().first();
}


void K3bAbstractWriter::slotProcessedSize( int made, int )
{
  if (!m_started) {
    m_lastWriteSpeedCalcTime = QTime::currentTime();
    m_lastWrittenBytes = made;
    m_started = true;
  }
  else {
    int elapsed = m_lastWriteSpeedCalcTime.msecsTo( QTime::currentTime() );
    int written = made - m_lastWrittenBytes;
    if( elapsed > 0 && written > 0 ) {
      m_lastWriteSpeedCalcTime = QTime::currentTime();
      m_lastWrittenBytes = made;
      emit writeSpeed( (int)((double)written * 1024.0 * 1000.0 / (double)elapsed) );
    }
  }
}


void K3bAbstractWriter::slotFinished( bool )
{
  m_started = false;
}

#include "k3babstractwriter.moc"
