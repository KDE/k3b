/***************************************************************************
                          k3bdiskinfodetector.h  -  description
                             -------------------
    begin                : Tue May 14 2002
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
#ifndef K3BDISKINFO_H
#define K3BDISKINFO_H


#include "../device/k3btoc.h"

#include <qstring.h>

class K3bDevice;

class K3bDiskInfo
{
 public:
  K3bDiskInfo()
    : empty(false), 
    cdrw(false), 
    appendable(false), 
    noDisk(true),
    isVideoDvd(false),
    isVCD(false),
    size(0),
    remaining(0),
    speed(0),
    sessions(0),
    tocType(UNKNOWN),
    valid(false),
    device(0)
    { }

  enum type { UNKNOWN, NODISC, AUDIO, DATA, MIXED, DVD };

  K3bToc toc;
  QString mediumManufactor;
  QString mediumType;
  QString sizeString;
  QString remainingString;

  bool empty;
  bool cdrw;
  bool appendable;
  bool noDisk;
  bool isVideoDvd;
  bool isVCD;

  unsigned long size;
  unsigned long remaining;

  int speed;
  int sessions;
  int tocType;

  // iso stuff
  QString isoId;
  QString isoSystemId;
  QString isoVolumeId;
  QString isoVolumeSetId;
  QString isoPublisherId;
  QString isoPreparerId;
  QString isoApplicationId;

  bool valid;

  K3bDevice* device;
};

#endif
