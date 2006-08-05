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

#ifndef _K3B_DATA_VERIFYING_JOB_H_
#define _K3B_DATA_VERIFYING_JOB_H_

#include <k3bjob.h>
#include "k3bdatadoc.h"


class K3bDataDoc;
class K3bIsoImager;
namespace K3bDevice {
  class DeviceHandler;
}


class K3bDataVerifyingJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bDataVerifyingJob( K3bJobHandler*, QObject* parent = 0, const char* name = 0 );
  ~K3bDataVerifyingJob();

  bool active() const;

 public slots:
  void start();
  void cancel();

  void setDoc( K3bDataDoc* );
  void setDevice( K3bDevice::Device* );
  void setImager( K3bIsoImager* );
  void setUsedMultisessionMode( int mode );

 private slots:
  void slotMediaReloaded( bool );
  void slotTocRead( K3bDevice::DeviceHandler* );
  void slotMd5JobFinished( bool );
  void slotDataTrackReaderFinished( bool success );
  void slotDataTrackReaderProgress( int percent );

 private:
  void readWrittenChecksum();
  void compareChecksums();
  void finishVerification( bool success );
  int imageSize() const;

  class Private;
  Private* d;
};

#endif
