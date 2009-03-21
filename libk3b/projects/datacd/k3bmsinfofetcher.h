/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef K3B_MSINFO_FETCHER_H
#define K3B_MSINFO_FETCHER_H

#include <k3bjob.h>

namespace K3b {

    class Process;
    namespace Device {
        class Device;
        class DeviceHandler;
    }

    class MsInfoFetcher : public Job
    {
        Q_OBJECT

    public:
        MsInfoFetcher( JobHandler*, QObject* parent = 0 );
        ~MsInfoFetcher();

        QString msInfo() const { return m_msInfo; }
        int lastSessionStart() const { return m_lastSessionStart; }
        int nextSessionStart() const { return m_nextSessionStart; }

    public Q_SLOTS:
        void start();
        void cancel();

        void setDevice( K3b::Device::Device* dev ) { m_device = dev; }

    private Q_SLOTS:
        void slotProcessExited();
        void slotMediaDetectionFinished( K3b::Device::DeviceHandler* );
        void getMsInfo();

    private:
        QString m_msInfo;
        int m_lastSessionStart;
        int m_nextSessionStart;
        QString m_collectedOutput;

        Process* m_process;
        Device::Device* m_device;

        bool m_canceled;
        bool m_dvd;
    };
}

#endif
