/***************************************************************************
                          k3bmp3track.cpp  -  description
                             -------------------
    begin                : Thu Jun 14 2001
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

#include "k3bwavetrack.h"
#include "../k3bglobals.h"


K3bWaveTrack::K3bWaveTrack( QList<K3bAudioTrack>* parent, const QString& filename )
  : K3bAudioTrack( parent, filename )
{
  m_filetype = K3b::WAV;

  // TODO: determine tracklength

}

K3bWaveTrack::~K3bWaveTrack()
{
}
