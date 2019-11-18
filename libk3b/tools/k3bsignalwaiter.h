/*
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_SIGNAL_WAITER_H_
#define _K3B_SIGNAL_WAITER_H_

#include "k3b_export.h"

#include <QObject>

namespace K3b {
    class Job;
}

namespace K3b {
    class SignalWaiter : public QObject
    {
        Q_OBJECT

    public:
        /**
         * Use this to synchronously wait for a signal.
         */
        LIBK3B_EXPORT static void waitForSignal( QObject* o, const char* signal );

        /**
         * Use this to synchronously wait for a job to finish.
         * If the job is not running at all this returns immediately.
         */
        LIBK3B_EXPORT static void waitForJob( Job* job );

    private Q_SLOTS:
        void slotSignal();

    private:
        SignalWaiter();
        ~SignalWaiter() override;

        class Private;
        Private* const d;
    };
}

#endif
