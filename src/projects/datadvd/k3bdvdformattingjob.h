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

#ifndef _K3B_DVD_FORMATTING_JOB_H_
#define _K3B_DVD_FORMATTING_JOB_H_


#include <k3bjob.h>


class KProcess;
namespace K3bCdDevice {
  class CdDevice;
  class DeviceHandler;
}


class K3bDvdFormattingJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bDvdFormattingJob( K3bJobHandler*, QObject* parent = 0, const char* name = 0 );
  ~K3bDvdFormattingJob();

  QString jobDescription() const;
  QString jobDetails() const;

 public slots:
  void start();

  /**
   * The devicehandler needs to have a valid NgDiskInfo
   */
  void start( K3bCdDevice::DeviceHandler* );

  void cancel();

  void setDevice( K3bCdDevice::CdDevice* );

  /**
   * One of: WRITING_MODE_INCR_SEQ, WRITING_MODE_RES_OVWR
   * Ignored for DVD+RW
   */
  void setMode( int );

  /**
   * Not all writers support this
   */
  void setQuickFormat( bool );

  /**
   * @param b If true empty DVDs will also be formatted
   */
  void setForce( bool b );

  /**
   * If set true the job ignores the global K3b setting
   * and does not eject the CD-RW after finishing
   */
  void setForceNoEject( bool );

 private slots:
  void slotStderrLine( const QString& );
  void slotProcessFinished( KProcess* );
  void slotDeviceHandlerFinished( K3bCdDevice::DeviceHandler* );
  void slotEjectingFinished( K3bCdDevice::DeviceHandler* );

 private:
  class Private;
  Private* d;
};


#endif
