/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDISKINFO_H
#define K3BDISKINFO_H


#include "k3btoc.h"
#include "k3bmsf.h"

#include <qstring.h>

namespace K3bCdDevice
{
  class CdDevice;

  class DiskInfo
  {
  public:
    DiskInfo()
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

    Toc toc;
    QString mediumManufactor;
    QString mediumType;

    bool empty;
    bool cdrw;
    bool appendable;
    bool noDisk;
    bool isVideoDvd;
    bool isVCD;

    K3b::Msf size;
    K3b::Msf remaining;

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

    CdDevice* device;
  };
};

typedef K3bCdDevice::DiskInfo K3bDiskInfo;

#endif
