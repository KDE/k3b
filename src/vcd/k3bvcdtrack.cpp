/***************************************************************************
                          k3bvcdtrack.cpp  -  description
                             -------------------
    begin                : Mon Nov 4 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#include "k3bvcdtrack.h"
#include "../tools/k3bglobals.h"

#include <kapplication.h>
#include <kconfig.h>

#include <qstring.h>
#include <qfileinfo.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <kdebug.h>


K3bVcdTrack::K3bVcdTrack( QPtrList<K3bVcdTrack>* parent, const QString& filename )
: m_file(filename)
{
  m_parent = parent;
}


K3bVcdTrack::~K3bVcdTrack()
{
}


unsigned long K3bVcdTrack::size() const
{
  return (m_file.size() + 2351) / 2352 * 2352;
}

void K3bVcdTrack::setMpegType(const QString& mt)
{
  m_mpegtype = mt;
}

void K3bVcdTrack::setMpegDuration(const QString& time)
{
  m_mpegduration = time;
}

void K3bVcdTrack::setMpegSize(const QString& size)
{
  m_mpegsize = size;
}

void K3bVcdTrack::setMpegDisplaySize(const QString& size)
{
  m_mpegdisplaysize = size;
}

void K3bVcdTrack::setMpegFps(const QString& fps)
{
  m_mpegfps = fps;
}

void K3bVcdTrack::setMpegMbps(const QString& mbps)
{
  m_mpegmbps = mbps;
}


int K3bVcdTrack::index() const
{
  int i = m_parent->find( this );
  if( i < 0 )
    kdDebug() << "(K3bVcdTrack) I'm not part of my parent!" << endl;
  return i;
}

