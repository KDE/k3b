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


#include "k3bmd5job.h"

#include <kmdcodec.h>
#include <klocale.h>
#include <kdebug.h>

#include <qfile.h>
#include <qtimer.h>


class K3bMd5Job::K3bMd5JobPrivate
{
public:
  K3bMd5JobPrivate() {
    finished = true;
    data = 0;
  }

  KMD5 md5;
  QFile file;
  QTimer timer;
  QString filename;
  bool finished;
  char* data;

  static const int BUFFERSIZE = 1024*20;
};


K3bMd5Job::K3bMd5Job( QObject* parent, const char* name )
  : K3bJob( parent, name )
{
  d = new K3bMd5JobPrivate;
  d->data = new char[K3bMd5JobPrivate::BUFFERSIZE];
  connect( &d->timer, SIGNAL(timeout()), 
	   this, SLOT(slotUpdate()) );
}


K3bMd5Job::~K3bMd5Job()
{
  delete [] d->data;
  delete d;
}


void K3bMd5Job::start()
{
  cancel();

  if( !QFile::exists( d->filename ) ) {
    emit infoMessage( i18n("Could not find file %1").arg(d->filename), ERROR );
    emit finished(false);
  }
  else {
    d->file.setName( d->filename );
    if( !d->file.open( IO_ReadOnly ) ) {
      emit infoMessage( i18n("Could not open file %1").arg(d->filename), ERROR );
      emit finished(false);
    }
    else {
      emit started();

      d->md5.reset();
      d->finished = false;
      d->timer.start(0);
    }
  }
}


void K3bMd5Job::cancel()
{
  if( !d->finished ) {
    stop();

    emit canceled();
    emit finished( false );
  }
}


void K3bMd5Job::setFile( const QString& filename )
{
  d->filename = filename;
}


void K3bMd5Job::slotUpdate()
{
  if( !d->finished ) {
    if( d->file.atEnd() ) {
      stop();
      emit percent( 100 );
      emit finished(true);
    }
    else {
      int read = d->file.readBlock( d->data, K3bMd5JobPrivate::BUFFERSIZE );
      if( read < 0 ) {
	emit infoMessage( i18n("Error while reading from file %1").arg(d->filename), ERROR );
	stop();
	emit finished(false);
      }
      else {
	d->md5.update( d->data, read );
	emit percent( (int)((double)d->file.at() * 100.0 / (double)d->file.size()) );
      }
    }
  }
}


QCString K3bMd5Job::hexDigest()
{
  if( d->finished )
    return d->md5.hexDigest();
  else
    return "";
}


QCString K3bMd5Job::base64Digest()
{
  if( d->finished )
    return d->md5.base64Digest();  
  else
    return "";

}


void K3bMd5Job::stop()
{
  d->file.close();
  d->timer.stop();
  d->finished = true;
}

#include "k3bmd5job.moc"
