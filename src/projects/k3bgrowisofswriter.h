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

#ifndef _K3B_GROWISOFS_WRITER_H_
#define _K3B_GROWISOFS_WRITER_H_

#include "k3babstractwriter.h"


namespace K3bCdDevice {
  class CdDevice;
  class DeviceHandler;
}
class KProcess;



class K3bGrowisofsWriter : public K3bAbstractWriter
{
  Q_OBJECT

 public:
  K3bGrowisofsWriter( K3bCdDevice::CdDevice*, QObject* parent = 0, const char* name = 0 );
  ~K3bGrowisofsWriter();

  bool write( const char* data, int len );

  int fd() const;
  bool closeFd();

 public slots:
  void start();
  void cancel();

  void setWritingMode( int );

  /**
   * set this to QString::null or an empty string to let the writer
   * read it's data from fd()
   */
  void setImageToWrite( const QString& );

 protected:
  bool prepareProcess();

 protected slots:
  void slotReceivedStderr( const QString& );
  void slotProcessExited( KProcess* );
  void slotEjectingFinished( K3bCdDevice::DeviceHandler* dh );
  void slotThroughput( int t );

 private:
  class Private;
  Private* d;
};

#endif
