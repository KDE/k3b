/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3blibdvdcss.h"

#include <k3bdevice.h>

#include <qfile.h>
#include <qcstring.h>

#include <dlfcn.h>


void* K3bLibDvdCss::s_libDvdCss = 0;
int K3bLibDvdCss::s_counter = 0;


extern "C" {
  struct dvdcss_s;
  typedef struct dvdcss_s* dvdcss_t;

  dvdcss_t (*dvdcss_open)(char*);
  int (*dvdcss_close)( dvdcss_t );
}


class K3bLibDvdCss::Private
{
public:
  Private()
    :dvd(0) {
  }

  dvdcss_t dvd;
};

K3bLibDvdCss::K3bLibDvdCss()
{
  d = new Private();
  s_counter++;
}


K3bLibDvdCss::~K3bLibDvdCss()
{
  close();
  delete d;
  s_counter--;
  if( s_counter == 0 ) {
    dlclose( s_libDvdCss );
    s_libDvdCss = 0;
  }
}


bool K3bLibDvdCss::open( K3bDevice::Device* dev )
{
  d->dvd = dvdcss_open( const_cast<char*>( QFile::encodeName(dev->blockDeviceName()).data() ) );
  return ( d->dvd != 0 );
}


void K3bLibDvdCss::close()
{
  if( d->dvd )
    dvdcss_close( d->dvd );
  d->dvd = 0;
}


K3bLibDvdCss* K3bLibDvdCss::create()
{
  if( s_libDvdCss == 0 ) {
    s_libDvdCss = dlopen( "libdvdcss.so", RTLD_NOW|RTLD_GLOBAL );
    if( s_libDvdCss ) {
      dvdcss_open = (dvdcss_t (*)(char*))dlsym( s_libDvdCss, "dvdcss_open" );
      dvdcss_close = (int (*)( dvdcss_t ))dlsym( s_libDvdCss, "dvdcss_close" );

      if( !dvdcss_open || !dvdcss_close ) {
	kdDebug() << "(K3bLibDvdCss) unable to resolve libdvdcss." << endl;
	dlclose( s_libDvdCss );
	s_libDvdCss = 0;
      }
    }
    else
      kdDebug() << "(K3bLibDvdCss) unable to load libdvdcss." << endl;
  }

  if( s_libDvdCss )
    return new K3bLibDvdCss();
  else
    return 0;
}
