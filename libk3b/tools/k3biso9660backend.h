/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_ISO9660_BACKEND_H_
#define _K3B_ISO9660_BACKEND_H_

#include <qstring.h>

#include "k3b_export.h"

namespace K3bDevice {
  class Device;
}

class K3bLibDvdCss;


class K3bIso9660Backend
{
 public:
  K3bIso9660Backend() {}
  virtual ~K3bIso9660Backend() {}

  virtual bool open() = 0;
  virtual void close() = 0;
  virtual bool isOpen() const = 0;
  virtual int read( unsigned int sector, char* data, int len ) = 0;
};


class K3bIso9660DeviceBackend : public K3bIso9660Backend
{
 public:
  LIBK3B_EXPORT K3bIso9660DeviceBackend( K3bDevice::Device* dev );
  ~K3bIso9660DeviceBackend();

  bool open();
  void close();
  bool isOpen() const { return m_isOpen; }
  int read( unsigned int sector, char* data, int len );

 private:
  K3bDevice::Device* m_device;
  bool m_isOpen;
};


class K3bIso9660FileBackend : public K3bIso9660Backend
{
 public:
  LIBK3B_EXPORT K3bIso9660FileBackend( const QString& filename );
  K3bIso9660FileBackend( int fd );
  ~K3bIso9660FileBackend();

  bool open();
  void close();
  bool isOpen() const;
  int read( unsigned int sector, char* data, int len );

 private:
  QString m_filename;
  int m_fd;
  bool m_closeFd;
};


class K3bIso9660LibDvdCssBackend : public K3bIso9660Backend
{
 public:
  LIBK3B_EXPORT K3bIso9660LibDvdCssBackend( K3bDevice::Device* );
  ~K3bIso9660LibDvdCssBackend();

  bool open();
  void close();
  bool isOpen() const;
  int read( unsigned int sector, char* data, int len );

 private:
  K3bDevice::Device* m_device;
  K3bLibDvdCss* m_libDvdCss;
};

#endif
