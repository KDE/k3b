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


#ifndef _K3B_MD5_JOB_H_
#define _K3B_MD5_JOB_H_

#include <k3bjob.h>
#include <qcstring.h>


class K3bMd5Job : public K3bJob
{
  Q_OBJECT

 public:
  K3bMd5Job( QObject* parent = 0, const char* name = 0 );
  ~K3bMd5Job();

  QCString hexDigest();
  QCString base64Digest();

 public slots:
  void start();
  void cancel();

  void setFile( const QString& filename );

 private slots:
  void slotUpdate();
  void stop();

 private:
  class K3bMd5JobPrivate;
  K3bMd5JobPrivate* d;
};

#endif
