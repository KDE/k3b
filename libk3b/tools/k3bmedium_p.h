/*

    SPDX-FileCopyrightText: 2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MEDIUM_P_H_
#define _K3B_MEDIUM_P_H_

#include "k3bmedium.h"

#include "k3bdiskinfo.h"
#include "k3btoc.h"
#include "k3bcdtext.h"
#include "k3biso9660.h"

#include <QSharedData>
#include <QList>

#include <KCddb/Cdinfo>


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
