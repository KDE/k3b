/***************************************************************************
                          k3babstractreader.cpp  -  description
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

#include "k3babstractreader.h"

K3bAbstractReader::K3bAbstractReader( QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_readDevice(0),
    m_readSpeed(1),
    m_readraw(false)
{
}


K3bAbstractReader::~K3bAbstractReader()
{
}

#include "k3babstractreader.moc"
