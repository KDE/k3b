/* 
 *
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


#ifndef _videodvd_H_
#define _videodvd_H_

#include <qstring.h>

#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>

class K3bIso9660Entry;
class K3bIso9660;
namespace K3bDevice
{
  class DeviceManager;
}

class kio_videodvdProtocol : public KIO::SlaveBase
{
public:
  kio_videodvdProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
  ~kio_videodvdProtocol();

  void mimetype( const KUrl& url );
  void stat( const KUrl& url );
  void get( const KUrl& url );
  void listDir( const KUrl& url );

private:
  K3bIso9660* openIso( const KUrl&, QString& plainIsoPath );
  KIO::UDSEntry createUDSEntry( const K3bIso9660Entry* e ) const;
  void listVideoDVDs();

  static K3bDevice::DeviceManager* s_deviceManager;
  static int s_instanceCnt;
};

#endif
