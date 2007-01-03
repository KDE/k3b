/*
 *
 * $Id: k3bemptydiscwaiter.cpp 606691 2006-11-21 12:15:21Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_DEBUG_H_
#define _K3B_DEBUG_H_

#include <qstring.h>

#include <k3bmsf.h>
#include <k3bdevice_export.h>

class K3bDebug;

typedef K3bDebug& (*K3BDBGFUNC)( K3bDebug& );

/**
 * K3bDebug as compared to KDebug does not need anything. No KInstance or whatever
 * and does not use anything except fprintf.
 * Thus, K3bDebug is fully thread-safe and safe in general
 */
class LIBK3BDEVICE_EXPORT K3bDebug
{
 public:
  ~K3bDebug();

  K3bDebug& operator<<( int );
  K3bDebug& operator<<( long );
  K3bDebug& operator<<( unsigned int );
  K3bDebug& operator<<( unsigned long );
  K3bDebug& operator<<( unsigned long long );
  K3bDebug& operator<<( char );
  K3bDebug& operator<<( float );
  K3bDebug& operator<<( double );
  K3bDebug& operator<<( const QString& );
  K3bDebug& operator<<( const QCString& );
  K3bDebug& operator<<( const char* );
  K3bDebug& operator<<( const K3b::Msf& );

  K3bDebug& operator<<( K3BDBGFUNC );

  static K3bDebug& k3bDebug();

 private:
  K3bDebug();
};

LIBK3BDEVICE_EXPORT K3bDebug& k3bDebug();
LIBK3BDEVICE_EXPORT K3bDebug& endl( K3bDebug& );

#endif
