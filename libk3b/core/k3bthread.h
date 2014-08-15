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


#ifndef _K3B_THREAD_H_
#define _K3B_THREAD_H_

#include "k3bdevicetypes.h"
#include "k3b_export.h"
#include <QtCore/QThread>


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
        Thread( ThreadJob* parent = 0 );
        ~Thread();

        void ensureDone();
        bool success() const;

        /**
         * waits until all running Thread have finished.
         * This is used by Application.
         */
        static void waitUntilFinished();

    protected:
        void run();

    private Q_SLOTS:
        void slotEnsureDoneTimeout();

    private:
        class Private;
        Private* d;
    };
}

#endif
