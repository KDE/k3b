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

#include <config.h>

#include <qcstring.h>
#include <qdatetime.h>
#include <qbitarray.h>
#include <qptrlist.h>

#include <kdebug.h>
#include <kinstance.h>
#include <kglobal.h>
#include <klocale.h>
#include <kurl.h>

#include <stdlib.h>

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3biso9660.h>
#include <k3biso9660backend.h>
#include <k3b_export.h>

#include "videodvd.h"

using namespace KIO;

extern "C"
{
  LIBK3B_EXPORT int kdemain( int argc, char **argv )
  {
    KInstance instance( "kio_videodvd" );

    kdDebug(7101) << "*** Starting kio_videodvd " << endl;

    if (argc != 4)
    {
      kdDebug(7101) << "Usage: kio_videodvd  protocol domain-socket1 domain-socket2" << endl;
      exit(-1);
    }

    kio_videodvdProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    kdDebug(7101) << "*** kio_videodvd Done" << endl;
    return 0;
  }
}



// FIXME: Does it really make sense to use a static device manager? Are all instances
// of videodvd started in another process?
K3bDevice::DeviceManager* kio_videodvdProtocol::s_deviceManager = 0;
int kio_videodvdProtocol::s_instanceCnt = 0;

kio_videodvdProtocol::kio_videodvdProtocol(const QCString &pool_socket, const QCString &app_socket)
    : SlaveBase("kio_videodvd", pool_socket, app_socket)
{
  kdDebug() << "kio_videodvdProtocol::kio_videodvdProtocol()" << endl;
  if( !s_deviceManager )
  {
    s_deviceManager = new K3bDevice::DeviceManager();
    s_deviceManager->setCheckWritingModes( false );
    s_deviceManager->scanBus();
  }
  s_instanceCnt++;
}


kio_videodvdProtocol::~kio_videodvdProtocol()
{
  kdDebug() << "kio_videodvdProtocol::~kio_videodvdProtocol()" << endl;
  s_instanceCnt--;
  if( s_instanceCnt == 0 )
  {
    delete s_deviceManager;
    s_deviceManager = 0;
  }
}


KIO::UDSEntry kio_videodvdProtocol::createUDSEntry( const K3bIso9660Entry* e ) const
{
  KIO::UDSEntry uds;
  KIO::UDSAtom a;

  a.m_uds = KIO::UDS_NAME;
  a.m_str = e->name();
  uds.append( a );

  a.m_uds = KIO::UDS_ACCESS;
  a.m_long = e->permissions();
  uds.append( a );

  a.m_uds = KIO::UDS_CREATION_TIME;
  a.m_long = e->date();
  uds.append( a );

  a.m_uds = KIO::UDS_MODIFICATION_TIME;
  a.m_long = e->date();
  uds.append( a );

  if( e->isDirectory() )
  {
    a.m_uds = KIO::UDS_FILE_TYPE;
    a.m_long = S_IFDIR;
    uds.append( a );

    a.m_uds = KIO::UDS_MIME_TYPE;
    a.m_str = "inode/directory";
    uds.append( a );
  }
  else
  {
    const K3bIso9660File* file = static_cast<const K3bIso9660File*>( e );

    a.m_uds = KIO::UDS_SIZE;
    a.m_long = file->size();
    uds.append( a );

    a.m_uds = KIO::UDS_FILE_TYPE;
    a.m_long = S_IFREG;
    uds.append( a );

    a.m_uds = KIO::UDS_MIME_TYPE;
    if( e->name().endsWith( "VOB" ) )
      a.m_str = "video/mpeg";
    else
      a.m_str = "unknown";
    uds.append( a );
  }

  return uds;
}


// FIXME: remember the iso instance for quicker something and search for the videodvd
//        in the available devices.
K3bIso9660* kio_videodvdProtocol::openIso( const KURL& url, QString& plainIsoPath )
{
  // get the volume id from the url
  QString volumeId = url.path().section( '/', 1, 1 );

  kdDebug() << "(kio_videodvdProtocol) searching for Video dvd: " << volumeId << endl;

  // now search the devices for this volume id
  // FIXME: use the cache created in listVideoDVDs
  for( QPtrListIterator<K3bDevice::Device> it( s_deviceManager->dvdReader() ); *it; ++it ) {
    K3bDevice::Device* dev = *it;
    K3bDevice::DiskInfo di = dev->diskInfo();

    // we search for a DVD with a single track.
    // this time let K3bIso9660 decide if we need dvdcss or not
    // FIXME: check for encryption and libdvdcss and report an error
    if( di.isDvdMedia() && di.numTracks() == 1 ) {
      K3bIso9660* iso = new K3bIso9660( dev );
      iso->setPlainIso9660( true );
      if( iso->open() && iso->primaryDescriptor().volumeId == volumeId ) {
	plainIsoPath = url.path().section( "/", 2, -1 ) + "/";
	kdDebug() << "(kio_videodvdProtocol) using iso path: " << plainIsoPath << endl;
	return iso;
      }
      delete iso;
    }
  }

  error( ERR_SLAVE_DEFINED, i18n("No VideoDVD found") );
  return 0;
}


void kio_videodvdProtocol::get(const KURL& url )
{
  kdDebug() << "kio_videodvd::get(const KURL& url)" << endl ;

  QString isoPath;
  if( K3bIso9660* iso = openIso( url, isoPath ) )
  {
    const K3bIso9660Entry* e = iso->firstIsoDirEntry()->entry( isoPath );
    if( e && e->isFile() )
    {
      const K3bIso9660File* file = static_cast<const K3bIso9660File*>( e );
      totalSize( file->size() );
      QByteArray buffer( 10*2048 );
      int read = 0;
      int cnt = 0;
      KIO::filesize_t totalRead = 0;
      while( (read = file->read( totalRead, buffer.data(), buffer.size() )) > 0 )
      {
        buffer.resize( read );
        data(buffer);
        ++cnt;
        totalRead += read;
        if( cnt == 10 )
        {
          cnt = 0;
          processedSize( totalRead );
        }
      }

      delete iso;

      data(QByteArray()); // empty array means we're done sending the data

      if( read == 0 )
        finished();
      else
        error( ERR_SLAVE_DEFINED, i18n("Read error.") );
    }
    else
      error( ERR_DOES_NOT_EXIST, url.path() );
  }
}


void kio_videodvdProtocol::listDir( const KURL& url )
{
  if( url.path() == "/" ) {
    listVideoDVDs();
  }
  else {
    QString isoPath;
    K3bIso9660* iso = openIso( url, isoPath );
    if( iso ) {
      const K3bIso9660Directory* mainDir = iso->firstIsoDirEntry();
      const K3bIso9660Entry* e = mainDir->entry( isoPath );
      if( e ) {
	if( e->isDirectory() ) {
	  const K3bIso9660Directory* dir = static_cast<const K3bIso9660Directory*>(e);
	  QStringList el = dir->entries();
	  el.remove( "." );
	  el.remove( ".." );
	  UDSEntryList udsl;
	  for( QStringList::const_iterator it = el.begin(); it != el.end(); ++it )
	    udsl.append( createUDSEntry( dir->entry( *it ) ) );
	  listEntries( udsl );
	  finished();
	}
	else {
	  error( ERR_CANNOT_ENTER_DIRECTORY, url.path() );
	}
      }
      else {
	error( ERR_CANNOT_ENTER_DIRECTORY, url.path() );
      }

      // for testing we always do the whole thing
      delete iso;
    }
  }
}


void kio_videodvdProtocol::listVideoDVDs()
{
  int cnt = 0;

  for( QPtrListIterator<K3bDevice::Device> it( s_deviceManager->dvdReader() ); *it; ++it ) {
    K3bDevice::Device* dev = *it;
    K3bDevice::DiskInfo di = dev->diskInfo();

    // we search for a DVD with a single track.
    if( di.isDvdMedia() && di.numTracks() == 1 ) {
      //
      // now do a quick check for VideoDVD.
      // - no dvdcss for speed
      // - only a check for the VIDEO_TS dir
      //
      K3bIso9660 iso( new K3bIso9660DeviceBackend(dev) );
      iso.setPlainIso9660( true );
      if( iso.open() && iso.firstIsoDirEntry()->entry( "VIDEO_TS" ) ) {
	// FIXME: cache the entry for speedup

        UDSEntryList udsl;
	KIO::UDSEntry uds;
	KIO::UDSAtom a;
	
	a.m_uds = KIO::UDS_NAME;
	a.m_str = iso.primaryDescriptor().volumeId;
	uds.append( a );

	a.m_uds = KIO::UDS_FILE_TYPE;
	a.m_long = S_IFDIR;
	uds.append( a );
	
	a.m_uds = KIO::UDS_MIME_TYPE;
	a.m_str = "inode/directory";
	uds.append( a );

	a.m_uds = KIO::UDS_ICON_NAME;
	a.m_str = "dvd_unmount";
	uds.append( a );

	udsl.append( uds );

	listEntries( udsl );

	++cnt;
      }
    }
  }

  if( cnt )
    finished();
  else
    error( ERR_SLAVE_DEFINED, i18n("No VideoDVD found") );
}


void kio_videodvdProtocol::stat( const KURL& url )
{
  if( url.path() == "/" ) {
    //
    // stat the root path
    //
    KIO::UDSEntry uds;
    KIO::UDSAtom a;
    
    a.m_uds = KIO::UDS_NAME;
    a.m_str = "/";
    uds.append( a );
    
    a.m_uds = KIO::UDS_FILE_TYPE;
    a.m_long = S_IFDIR;
    uds.append( a );

    a.m_uds = KIO::UDS_MIME_TYPE;
    a.m_str = "inode/directory";
    uds.append( a );

    statEntry( uds );
    finished();
  }
  else {
    QString isoPath;
    K3bIso9660* iso = openIso( url, isoPath );
    if( iso ) {
      const K3bIso9660Entry* e = iso->firstIsoDirEntry()->entry( isoPath );
      if( e ) {
	statEntry( createUDSEntry( e ) );
	finished();
      }
      else
	error( ERR_DOES_NOT_EXIST, url.path() );
      
      delete iso;
    }
  }
}


// FIXME: when does this get called? It seems not to be used for the files.
void kio_videodvdProtocol::mimetype( const KURL& url )
{
  if( url.path() == "/" ) {
    error( ERR_UNSUPPORTED_ACTION, "mimetype(/)" );
    return;
  }

  QString isoPath;
  K3bIso9660* iso = openIso( url, isoPath );
  if( iso )
  {
    const K3bIso9660Entry* e = iso->firstIsoDirEntry()->entry( isoPath );
    if( e )
    {
      if( e->isDirectory() )
        mimeType( "inode/directory" );
      else if( e->name().endsWith( ".VOB" ) )
      {
        mimetype( "video/mpeg" );
      }
      else
      {
        // send some data
        const K3bIso9660File* file = static_cast<const K3bIso9660File*>( e );
        QByteArray buffer( 10*2048 );
        int read = file->read( 0, buffer.data(), buffer.size() );
        if( read > 0 )
        {
          buffer.resize( read );
          data(buffer);
          data(QByteArray());
          finished();
          // FIXME: do we need to emit finished() after emitting the end of data()?
        }
        else
          error( ERR_SLAVE_DEFINED, i18n("Read error.") );
      }
    }
    delete iso;
  }
}
