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

#include "k3bdebug.h"

#include <stdio.h>


K3bDebug::K3bDebug()
{
}


K3bDebug::~K3bDebug()
{
}


K3bDebug& K3bDebug::operator<<( int i )
{
  fprintf( stderr, "%i", i );
  return *this;
}


K3bDebug& K3bDebug::operator<<( long l )
{
  fprintf( stderr, "%li", l );
  return *this;
}


K3bDebug& K3bDebug::operator<<( unsigned int i )
{
  fprintf( stderr, "%u", i );
  return *this;
}


K3bDebug& K3bDebug::operator<<( unsigned long l )
{
  fprintf( stderr, "%lu", l );
  return *this;
}


K3bDebug& K3bDebug::operator<<( unsigned long long l )
{
  fprintf( stderr, "%llu", l );
  return *this;
}


K3bDebug& K3bDebug::operator<<( char c )
{
  fprintf( stderr, "%c", c );
  return *this;
}


K3bDebug& K3bDebug::operator<<( float f )
{
  fprintf( stderr, "%f", f );
  return *this;
}


K3bDebug& K3bDebug::operator<<( double d )
{
  fprintf( stderr, "%f", d );
  return *this;
}


K3bDebug& K3bDebug::operator<<( const QString& s )
{
  fprintf( stderr, "%s", s.utf8().data() );
  return *this;
}


K3bDebug& K3bDebug::operator<<( const QCString& s )
{
  fprintf( stderr, "%s", s.data() );
  return *this;
}


K3bDebug& K3bDebug::operator<<( const char* s )
{
  fprintf( stderr, "%s", s );
  return *this;
}


K3bDebug& K3bDebug::operator<<( const K3b::Msf& msf )
{
  return *this << msf.toString();
}


K3bDebug& K3bDebug::operator<<( K3BDBGFUNC f )
{
  return f( *this );
}


K3bDebug& K3bDebug::k3bDebug()
{
  static K3bDebug s_debug;
  return s_debug;
}



K3bDebug& k3bDebug()
{
  return K3bDebug::k3bDebug();
}


K3bDebug& endl( K3bDebug& s )
{
  return s << '\n';
}
