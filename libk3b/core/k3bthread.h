/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_THREAD_H_
#define _K3B_THREAD_H_

#include <qthread.h>
#include <k3bdevicetypes.h>


class K3bThreadJob;
namespace K3bDevice {
    class Device;
}

/**
 * \warning This class is internal to K3bThreadJob
 *
 * See K3bThreadJob for more information.
 */
class K3bThread : public QThread
{
    Q_OBJECT

public:
    K3bThread( K3bThreadJob* parent = 0 );
    ~K3bThread();

    void ensureDone();
    bool success() const;

    /**
     * waits until all running K3bThread have finished.
     * This is used by K3bApplication.
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

#endif
