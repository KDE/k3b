/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
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

#include <config.h>

#include "k3bencodingconverter.h"

#include <qwidget.h>
#include <qlayout.h>

#include <kdebug.h>

#ifdef HAVE_ICONV_H
#include <langinfo.h>
#include <iconv.h>
#endif


class K3bEncodingConverter::Private
{
 public:
  iconv_t ic;
  QString localEncoding;
  QString lastEncoding;
};


K3bEncodingConverter::K3bEncodingConverter()
{
  d = new Private;
#ifdef HAVE_ICONV_H
  char* codec = nl_langinfo( CODESET );
  d->localEncoding = QString::fromLocal8Bit( codec );
  kdDebug() << "(K3bDataUrlAddingDialog) using locale codec: " << codec << endl;
  d->ic = ::iconv_open( "UCS-2BE", codec );
#endif
}


K3bEncodingConverter::~K3bEncodingConverter()
{
#ifdef HAVE_ICONV_H
  ::iconv_close( d->ic );
#endif
  delete d;
}


bool K3bEncodingConverter::encodedLocally( const QCString& s )
{
#ifdef HAVE_ICONV_H
  QCString utf8Encoded( s.length()*2 );
#if defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
  const char* in = s.data();
#else
  char* in = s.data();
#endif
  char* out = utf8Encoded.data();
  size_t inSize = s.length();
  size_t outSize = utf8Encoded.size();
  return( (size_t)-1 != ::iconv( d->ic, &in, &inSize, &out, &outSize ) );
#else
  return true;
#endif
}


bool K3bEncodingConverter::fixEncoding( const QCString& s, QCString& result, QWidget* parent, bool cache )
{
#ifdef IMPLEMENT_THIS_METHOD // HAVE_ICONV_H
  if( !d->lastEncoding.isEmpty() ) {
    //
    // try converting with the last encoding
    //
    if( convert( s, result, d->lastEncoding, d->localEncoding )
	&& encodedLocally( result ) ) {
      return true;
    }
  }



  if( cache ) {

  }
  else
    d->lastEncoding = QString::null;
#else
  return false;
#endif
}


bool K3bEncodingConverter::convert( const QCString& s, QCString& result, const QString& from, const QString& to )
{
  bool r = false;

#ifdef HAVE_ICONV_H
  iconv_t ic = ::iconv_open( to.local8Bit(), from.local8Bit() );

  result.resize( s.length() * 2 );
#if defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
  const char* in = s.data();
#else
  char* in = s.data();
#endif
  char* out = result.data();
  size_t inSize = s.length();
  size_t outSize = result.size();

  if( (size_t)-1 != ::iconv( ic, &in, &inSize, &out, &outSize ) )
    r = true;

  ::iconv_close( ic );
#endif

  return r;
}
