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

K3bAbstractWriter::K3bAbstractWriter( QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_burnDevice(0),
    m_burnSpeed(1),
    m_burnproof(false),
    m_simulate(false)
{
}


K3bAbstractWriter::~K3bAbstractWriter()
{
}

#include "k3babstractwriter.moc"
