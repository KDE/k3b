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

#ifndef _K3BDEVICE_WRAPPER_QIODEVICE_H_
#define _K3BDEVICE_WRAPPER_QIODEVICE_H_

#include <qiodevice.h>

namespace K3bCdDevice {
  class CdDevice;
}


/**
 * QIODevice which wrapps a sector area. Most likely you will
 * set the borders of a track via setTrackBorders. The device then will
 * handle the first sector as position 0 (at(0))
 * Without setting the borders size() and atEnd() are useless.
 */
class K3bDeviceWrapperQIODevice : public QIODevice
{
 public:
  K3bDeviceWrapperQIODevice( K3bCdDevice::CdDevice* );
  ~K3bDeviceWrapperQIODevice();

  void setTrackBorders( unsigned long start, unsigned long end );

  bool open( int mode );
  void close();
  void flush();
  Offset at() const;
  bool at( Offset pos );
  bool atEnd() const;
  Q_LONG readBlock( char * data, Q_ULONG maxlen );
  Q_LONG writeBlock( const char * data, Q_ULONG len );
  int getch();
  int putch( int ch );
  int ungetch( int ch );
  Offset size() const;

 private:
  K3bCdDevice::CdDevice* m_device;

  class Private;
  Private* d;
};

#endif
