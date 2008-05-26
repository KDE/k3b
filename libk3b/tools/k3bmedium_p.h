/* 
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_MEDIUM_P_H_
#define _K3B_MEDIUM_P_H_

#include <QtCore/QSharedData>
#include <QtCore/QList>

#include <k3bdiskinfo.h>
#include <k3btoc.h>
#include <k3bcdtext.h>
#include <k3biso9660.h>

#include <libkcddb/cdinfo.h>


/**
 * Internal class used by K3bMedium
 */
class K3bMediumPrivate : public QSharedData
{
public:
    K3bMediumPrivate();

    K3bDevice::Device* device;
    K3bDevice::DiskInfo diskInfo;
    K3bDevice::Toc toc;
    K3bDevice::CdText cdText;
    QList<int> writingSpeeds;
    K3bIso9660SimplePrimaryDescriptor isoDesc;
    int content;

    KCDDB::CDInfo cddbInfo;
};

#endif
