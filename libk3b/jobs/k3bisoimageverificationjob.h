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

#ifndef _K3B_ISOIMAGE_VERIFICATION_JOB_H_
#define _K3B_ISOIMAGE_VERIFICATION_JOB_H_

#include <k3bjob.h>

namespace K3bDevice {
  class Device;
}


class K3bIsoImageVerificationJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bIsoImageVerificationJob( K3bJobHandler*, QObject* parent = 0, const char* name = 0 );
  ~K3bIsoImageVerificationJob();

 public slots:
  void start();
  void cancel();
  void setDevice( K3bDevice::Device* dev );
  void setImageFileName( const QString& f );

 private slots:
  void slotMediaReloaded( bool success );
  void slotMd5JobFinished( bool success );
  void slotMd5JobProgress( int p );
  void finishVerification( bool success );

 private:
  class Private;
  Private* d;
};

#endif
