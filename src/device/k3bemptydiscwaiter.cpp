/***************************************************************************
                          k3bemptydiscwaiter.cpp  -  description
                             -------------------
    begin                : Mon Oct 22 2001
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

#include "k3bemptydiscwaiter.h"
#include "k3bdevice.h"

#include <qtimer.h>


K3bEmptyDiscWaiter::K3bEmptyDiscWaiter( K3bDevice* device )
{
  m_timer = new QTimer( this );
  m_device = device;
}


K3bEmptyDiscWaiter::~K3bEmptyDiscWaiter()
{
}

void K3bEmptyDiscWaiter::waitForEmptyDisc()
{
  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotTestForEmptyCd()) );
  m_timer->start(100);
}


void K3bEmptyDiscWaiter::cancel()
{
  m_timer->stop();
  m_timer->disconnect();
  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotDelayedDestruction()) );
  m_timer->start(0);

  emit canceled();
}


void K3bEmptyDiscWaiter::slotTestForEmptyCd()
{
  if( m_device->isReady() == 0 ) // OK
    {
      long length;
      if( m_device->cdCapacity( &length ) )
	  if( length > 0 ) {
	    m_timer->stop();
	    m_timer->disconnect();
	    connect( m_timer, SIGNAL(timeout()), this, SLOT(slotDelayedDestruction()) );
	    m_timer->start(0);

	    emit discReady();
	  }
    }
}


void K3bEmptyDiscWaiter::slotDelayedDestruction()
{
  delete this;
}
