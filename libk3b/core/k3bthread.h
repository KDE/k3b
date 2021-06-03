/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_THREAD_H_
#define _K3B_THREAD_H_

#include "k3bdevicetypes.h"
#include "k3b_export.h"
#include <QThread>


namespace K3b {
    namespace Device {
        class Device;
    }
    class ThreadJob;

    /**
     * \warning This class is internal to ThreadJob
     *
     * See ThreadJob for more information.
     */
    class LIBK3B_EXPORT Thread : public QThread
    {
        Q_OBJECT

    public:
        explicit Thread( ThreadJob* parent = 0 );
        ~Thread() override;

        void ensureDone();
        bool success() const;

        /**
         * waits until all running Thread have finished.
         * This is used by Application.
         */
        static void waitUntilFinished();

    protected:
        void run() override;

    private Q_SLOTS:
        void slotEnsureDoneTimeout();

    private:
        class Private;
        Private* d;
    };
}

#endif
