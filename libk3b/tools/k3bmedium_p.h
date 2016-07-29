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

#include "k3bmedium.h"

#include "k3bdiskinfo.h"
#include "k3btoc.h"
#include "k3bcdtext.h"
#include "k3biso9660.h"

#include <libkcddb/cdinfo.h>


namespace K3b {
    /**
     * Internal class used by Medium
     */
    class MediumPrivate : public QSharedData
    {
    public:
        MediumPrivate();

        Device::Device* device;
        Device::DiskInfo diskInfo;
        Device::Toc toc;
        Device::CdText cdText;
        QList<int> writingSpeeds;
        Iso9660SimplePrimaryDescriptor isoDesc;
        Medium::MediumContents content;

        KCDDB::CDInfo cddbInfo;
    };
}

#endif
