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

#include <klocale.h>

K3bAbstractWriter::K3bAbstractWriter( K3bDevice* dev, QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_burnDevice(dev),
    m_burnSpeed(1),
    m_burnproof(false),
    m_simulate(false)
{
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


void K3bAbstractWriter::createEstimatedWriteSpeed( int made, bool firstCall )
{
  if (firstCall) {
    m_lastWriteSpeedCalcTime = QTime::currentTime();
    m_firstWriteSpeedCalcTime = QTime::currentTime();
    m_lastWrittenBytes = made;
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


void K3bAbstractWriter::createAverageWriteSpeedInfoMessage()
{
  double speed = (double)m_lastWrittenBytes * 1024.0 / (double)m_firstWriteSpeedCalcTime.secsTo( m_lastWriteSpeedCalcTime );
  emit infoMessage( i18n("Average overall write speed: %1 kb/s (%2x)").arg((int)speed).arg(speed/150.0, 0, 'g', 2), INFO );
}

#include "k3babstractwriter.moc"
