/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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

    QMutex mutex;

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
    void run();

private:
    MediaCache::DeviceEntry* m_deviceEntry;
};

#endif
