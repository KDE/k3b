/*

    SPDX-FileCopyrightText: 2005-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef _K3B_MEDIA_CACHE_P_H_
#define _K3B_MEDIA_CACHE_P_H_

#include "k3bmediacache.h"

class K3b::MediaCache::DeviceEntry
{
public:
    DeviceEntry( MediaCache* cache, Device::Device* dev );
    ~DeviceEntry();

    Medium medium;

    int blockedId;

    QMutex readMutex;
    QMutex writeMutex;

    MediaCache::PollThread* thread;

    MediaCache* cache;

    void clear() {
        medium.reset();
    }
};


class K3b::MediaCache::PollThread : public QThread
{
    Q_OBJECT

public:
    PollThread( MediaCache::DeviceEntry* de )
        : m_deviceEntry( de ) {}

Q_SIGNALS:
    void mediumChanged( K3b::Device::Device* dev );
    void checkingMedium( K3b::Device::Device* dev, const QString& );

protected:
    void run() override;

private:
    MediaCache::DeviceEntry* m_deviceEntry;
};

#endif
