/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DVD_BOOKTYPE_JOB_H_
#define _K3B_DVD_BOOKTYPE_JOB_H_


#include <qprocess.h>
#include <k3bjob.h>


namespace K3bDevice {
  class Device;
  class DeviceHandler;
}


/**
 * This job can change the compatibility bit of DVD+R(W) media
 * with supported dvd writers.
 */
class K3bDvdBooktypeJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bDvdBooktypeJob( K3bJobHandler*, QObject* parent = 0 );
  ~K3bDvdBooktypeJob();

  QString jobDescription() const;
  QString jobDetails() const;

  /**
   * @list SET_MEDIA_DVD_ROM Change media identification on current media to DVD-ROM.
   * @list SET_MEDIA_DVD_R_W Change media identification on current media to DVD+R or DVD+RW.
   * @list SET_UNIT_DVD_ROM_ON_NEW_DVD_R Set the drive to write DVD-ROM specification on future written DVD+R discs.
   * @list SET_UNIT_DVD_ROM_ON_NEW_DVD_RW Set the drive to write DVD-ROM specification on future written DVD+RW discs.
   * @list SET_UNIT_DVD_R_ON_NEW_DVD_R Set the drive to write DVD+R specification on future written DVD+R discs.
   * @list SET_UNIT_DVD_RW_ON_NEW_DVD_RW Set the drive to write DVD+RW specification on future written DVD+RW discs.
   */
  enum Action {
    SET_MEDIA_DVD_ROM,
    SET_MEDIA_DVD_R_W,
    SET_UNIT_DVD_ROM_ON_NEW_DVD_R,
    SET_UNIT_DVD_ROM_ON_NEW_DVD_RW,
    SET_UNIT_DVD_R_ON_NEW_DVD_R,
    SET_UNIT_DVD_RW_ON_NEW_DVD_RW
  };

 public Q_SLOTS:
  void start();

  /**
   * The devicehandler needs to have a valid NgDiskInfo
   * Use this to prevent the job from searching a media.
   */
  void start( K3bDevice::DeviceHandler* );

  void cancel();

  void setDevice( K3bDevice::Device* );

  void setAction( int a ) { m_action = a; }

  /**
   * If set true the job ignores the global K3b setting
   * and does not eject the CD-RW after finishing
   */
  void setForceNoEject( bool );

 private Q_SLOTS:
  void slotStderrLine( const QString& );
  void slotProcessFinished( int, QProcess::ExitStatus );
  void slotDeviceHandlerFinished( K3bDevice::DeviceHandler* );
  void slotEjectingFinished( K3bDevice::DeviceHandler* );

 private:
  void startBooktypeChange();

  int m_action;

  class Private;
  Private* d;
};


#endif
