/*
 *
 * Copyright (C) 2003 Klaus-Dieter Krannich <kd@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef K3BDISKINFO_THREAD_H
#define K3BDISKINFO_THREAD_H

#include <qthread.h>
#include <qevent.h>

#include "k3bdiskinfo.h"
#include "k3bdiskinfodetector.h"

namespace K3bCdDevice
{
  class DiskInfoDetector;

  class DiskInfoThread : public QThread
  {

  public:
    DiskInfoThread(  QObject *, CdDevice *,DiskInfo *);
    ~DiskInfoThread();

  public:
    virtual void run();

  private:
    void fetchInfo();
    void fetchSizeInfo();
    void finish(bool);

  private:
    QObject *m_parent;
    CdDevice *m_device;
    DiskInfo *m_info;
  };
};

#endif
