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

#ifndef _K3B_DATA_VERIFYING_JOB_H_
#define _K3B_DATA_VERIFYING_JOB_H_

#include <k3bjob.h>

class K3bDataDoc;
namespace KIO {
  class Job;
}


class K3bDataVerifyingJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bDataVerifyingJob( QObject* parent = 0, const char* name = 0 );
  ~K3bDataVerifyingJob();

 public slots:
  void start();
  void cancel();

  void setDoc( K3bDataDoc* );
  void setDevice( K3bCdDevice::CdDevice* );

 private slots:
  void slotMediaReloaded( bool );
  void slotMountFinished( KIO::Job* job );
  void slotUnmountFinished( KIO::Job* job );
  void slotMd5JobFinished( bool );
  void slotMd5JobProgress( int );

 private:
  void compareNextFile();
  void finishVerification( bool success );

  class Private;
  Private* d;
};

#endif
