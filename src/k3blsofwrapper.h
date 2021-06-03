/*
    SPDX-FileCopyrightText: 2006 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_LSOF_WRAPPER_H_
#define _K3B_LSOF_WRAPPER_H_

#include <QList>
#include <QString>

namespace K3b {
    namespace Device {
        class Device;
    }

    class LsofWrapper
    {
    public:
        LsofWrapper();
        ~LsofWrapper();

        /**
         * Checks which processes currently have an open file descriptor
         * to the device.
         *
         * \return true if lsof was successfully called.
         */
        bool checkDevice( Device::Device* );

        struct Process {
            QString name;
            int pid;
        };

        /**
         * \return A list of all applications that had an open
         * handle on the device used in the last successful call
         * to checkDevice.
         */
        const QList<Process>& usingApplications() const;

    private:
        bool findLsofExecutable();

        class Private;
        Private* d;
    };
}

#endif

