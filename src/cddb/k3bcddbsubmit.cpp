/***************************************************************************
                          k3bcddbsubmit.cpp  -  description
                             -------------------
    begin                : Sun Oct 7 2001
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

#include "k3bcddbsubmit.h"

#include <qtimer.h>


K3bCddbSubmit::K3bCddbSubmit( QObject* parent, const char* name )
  : QObject( parent, name )
{
}


K3bCddbSubmit::~K3bCddbSubmit()
{
}


void K3bCddbSubmit::submit( const K3bCddbResultEntry& entry )
{
  m_resultEntry = entry;

  if( m_resultEntry.rawData.isEmpty() )
    createDataStream( m_resultEntry );

  QTimer::singleShot( 0, this, SLOT(doSubmit()) );
}


void K3bCddbSubmit::createDataStream( K3bCddbResultEntry& )
{
  // FIXME
}

#include "k3bcddbsubmit.moc"
