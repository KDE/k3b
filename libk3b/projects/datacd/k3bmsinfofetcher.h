/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_MSINFO_FETCHER_H
#define K3B_MSINFO_FETCHER_H

#include "k3bjob.h"

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
        explicit MsInfoFetcher( JobHandler*, QObject* parent = 0 );
        ~MsInfoFetcher() override;

        QString msInfo() const { return m_msInfo; }
        int lastSessionStart() const { return m_lastSessionStart; }
        int nextSessionStart() const { return m_nextSessionStart; }

    public Q_SLOTS:
        void start() override;
        void cancel() override;

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
