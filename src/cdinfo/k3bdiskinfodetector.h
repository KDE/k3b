/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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



#ifndef K3BDISKINFO_DETECTOR_H
#define K3BDISKINFO_DETECTOR_H

#include <qobject.h>

#include <device/k3bdiskinfo.h>
#include <device/k3bdevicehandler.h>
#include <device/k3bcdtext.h>


class K3bIso9660;


namespace K3bCdDevice
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

    const DiskInfo& diskInfo() const;
    const NextGenerationDiskInfo& ngDiskInfo() const;
    const AlbumCdText& cdText() const;
    const Toc& toc() const;

    const K3bIso9660* iso9660() const;

  public slots:
    void detect( CdDevice* dev );

  signals:
    void diskInfoReady( K3bCdDevice::DiskInfoDetector* );

  private slots:
    void slotDeviceHandlerFinished(K3bCdDevice::DeviceHandler *);
    void finish(bool success);
    void fetchExtraInfo();

  protected:
    class Private;
    Private* d;
  };
}

typedef K3bCdDevice::DiskInfoDetector K3bDiskInfoDetector;

#endif
