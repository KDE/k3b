/***************************************************************************
                          k3bwavetrack.h  -  description
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

#ifndef K3BWAVETRACK_H
#define K3BWAVETRACK_H


#include "k3baudiotrack.h"

/**
  *@author Sebastian Trueg
  */

class K3bWaveTrack : public K3bAudioTrack
{
 public:
  K3bWaveTrack( QList<K3bAudioTrack>* parent, const QString& filename );
  ~K3bWaveTrack();
};

#endif
