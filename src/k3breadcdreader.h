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


#ifndef _K3B_READCD_READER_H_
#define _K3B_READCD_READER_H_

#include <k3bjob.h>


class K3bProcess;
class KProcess;
class K3bExternalBin;
namespace K3bCdDevice {
  class CdDevice;
}
namespace K3b {
  class Msf;
}


class K3bReadcdReader : public K3bJob
{
  Q_OBJECT

 public:
  K3bReadcdReader( QObject* parent = 0, const char* name = 0 );
  ~K3bReadcdReader();

  bool active() const;

 public slots:
  void start();
  void cancel();

  void setReadDevice( K3bCdDevice::CdDevice* dev ) { m_readDevice = dev; }

  /** 0 means MAX */
  void setReadSpeed( int s ) { m_speed = s; }
  void setDisableCorrection( bool b ) { m_noCorr = b; }

  /** default: true */
  void setAbortOnError( bool b ) { m_noError = !b; }
  void setC2Scan( bool b ) { m_c2Scan = b; }
  void setClone( bool b ) { m_clone = b; }

  void setSectorRange( const K3b::Msf&, const K3b::Msf& );

  void setImagePath( const QString& p ) { m_imagePath = p; }

  /**
   * the data gets written directly into fd instead of the imagefile.
   * Be aware that this only makes sense before starting the job.
   * To disable just set fd to -1
   */
  void writeToFd( int fd );

 private slots:
  void slotStdLine( const QString& line );
  void slotProcessExited(KProcess*);

 private:
  bool m_noCorr;
  bool m_clone;
  bool m_noError;
  bool m_c2Scan;
  int m_speed;

  K3bCdDevice::CdDevice* m_readDevice;

  QString m_imagePath;

  class Private;
  Private* d;
};

#endif
