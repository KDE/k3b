/* 
 *
 * $Id$
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

#ifndef _K3B_CDDA2WAV_READER_H_
#define _K3B_CDDA2WAV_READER_H_

#include <k3bjob.h>

class K3Process;
namespace K3bDevice {
  class Device;
};


/**
 * An Audio CD reader completely based on cdda2wav.
 * It does not use K3bDevice::Device but parses the track offsets
 * from the cdda2wav output.
 */
class K3bCdda2wavReader : public K3bJob
{
  Q_OBJECT

 public:
  K3bCdda2wavReader( QObject* parent = 0 );
  ~K3bCdda2wavReader();

  bool active() const;

 public slots:
  void start();
  void start( bool onlyReadInfo );
  void cancel();

  void setReadDevice( K3bDevice::Device* dev ) { m_device = dev; }
  void setImagePath( const QString& p ) { m_imagePath = p; }

  /**
   * the data gets written directly into fd instead of the imagefile.
   * Be aware that this only makes sense before starting the job.
   * To disable just set fd to -1
   */
  void writeToFd( int fd );

 private slots:
  void slotProcessLine( const QString& );
  void slotProcessExited( K3Process* );

 private:
  K3bDevice::Device* m_device;

  QString m_imagePath;

  class Private;
  Private* d;
};

#endif
