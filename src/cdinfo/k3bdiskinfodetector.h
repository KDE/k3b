/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef K3BDISKINFO_DETECTOR_H
#define K3BDISKINFO_DETECTOR_H

#include <qobject.h>

#include <k3bdiskinfo.h>
#include <k3bdevicehandler.h>
#include <k3bcdtext.h>


class K3bIso9660;


namespace K3bDevice
{
  class DeviceHandler;

  /**
   * The DiskInfoDetector gets as much information about a medium 
   * as possible.
   */
  class DiskInfoDetector : public QObject
  {
    Q_OBJECT

  public:
    DiskInfoDetector( QObject* parent = 0 );
    ~DiskInfoDetector();

    //    const DiskInfo& diskInfo() const;
    const DiskInfo& diskInfo() const;
    const CdText& cdText() const;
    const Toc& toc() const;

    const K3bIso9660* iso9660() const;

    /**
     * This is a temp solution. We should introduce some
     * contents representing class
     */
    bool isVideoDvd() const;

    /**
     * This is a temp solution. We should introduce some
     * contents representing class
     */
    bool isVideoCd() const;

    Device* device() const;

  public slots:
    void detect( Device* dev );

  signals:
    void diskInfoReady( K3bDevice::DiskInfoDetector* );

  private slots:
    void slotDeviceHandlerFinished(K3bDevice::DeviceHandler *);
    void finish(bool success);
    void fetchExtraInfo();

  protected:
    class Private;
    Private* d;
  };
}

typedef K3bDevice::DiskInfoDetector K3bDiskInfoDetector;

#endif
