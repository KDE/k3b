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

#include "k3bsmartinstancereuser.h"

#include <dcopref.h>
#include <kurl.h>
#include <kdebug.h>
#include <kcmdlineargs.h>
#include <kurl.h>

#include <qcstring.h>


K3bSmartInstanceReuser::K3bSmartInstanceReuser( KCmdLineArgs* args )
  : DCOPClient(),
    m_args( args )
{
  attach();
}


K3bSmartInstanceReuser::~K3bSmartInstanceReuser()
{
}


DCOPRef K3bSmartInstanceReuser::findInstance()
{
  QCStringList apps = registeredApplications();
  for( QCStringList::const_iterator it = apps.begin(); it != apps.end(); ++it ) {
    if( !qstrncmp( "k3b", (*it).data(), 3 ) ) {
      kdDebug() << "found K3b: " << *it << endl;
      QByteArray replyData;
      QCString replyType;
      if( call( *it, "K3bInterface", "blocked()", QByteArray(), replyType, replyData, false, -1 ) ) {
	QDataStream ds(replyData, IO_ReadOnly);
	bool blocked = false;
	ds >> blocked;
	if( blocked )
	  kdDebug() << *it << " is blocked" << endl;
	else {
	  kdDebug() << *it << " is not blocked" << endl;
	  return DCOPRef( *it, "K3bInterface" );
	}
      }
      else {
	kdDebug() << "call to blocked failed." << endl;
      }
    }
  }

  // return null ref
  return DCOPRef();
}


void K3bSmartInstanceReuser::reuseInstance( DCOPRef& instance )
{

  //
  // We found an unblocked instance of K3b
  //
  // TODO: raise the found instance to the foreground and switch to the desktop.


  //
  // handle the commandline args
  //

  QByteArray replyData;
  QCString replyType;
  bool projectCreated = false;
  if( m_args->isSet( "datacd" ) ) {
    // create new data project and add all arguments
    if( instance.call( "createDataCDProject()" ).isValid() )
      projectCreated = true;
  }
  else if( m_args->isSet( "audiocd" ) ) {
    // create new audio project and add all arguments
    if( instance.call( "createAudioCDProject()").isValid() )
      projectCreated = true;
  }
  else if( m_args->isSet( "mixedcd" ) ) {
    // create new audio project and add all arguments
    if( instance.call( "createMixedCDProject()").isValid() )
      projectCreated = true;
  }
  else if( m_args->isSet( "videocd" ) ) {
    // create new audio project and add all arguments
    if( instance.call( "createVideoCDProject()").isValid() )
      projectCreated = true;
  }
  else if( m_args->isSet( "emovixcd" ) ) {
    // create new audio project and add all arguments
    if( instance.call( "createMovixCDProject()").isValid() )
      projectCreated = true;
  }
  else if( m_args->isSet( "datadvd" ) ) {
    // create new audio project and add all arguments
    if( instance.call( "createDataDVDProject()").isValid() )
      projectCreated = true;
  }
  else if( m_args->isSet( "emovixdvd" ) ) {
    // create new audio project and add all arguments
    if( instance.call( "createMovixDVDProject()").isValid() )
      projectCreated = true;
  }
  else if( m_args->isSet( "videodvd" ) ) {
    // create new audio project and add all arguments
    if( instance.call( "createVideoDVDProject()").isValid() )
      projectCreated = true;
  }
      

  if( !projectCreated && m_args->isSet( "cdimage" ) ) {
    if ( m_args->count() == 1 )
      instance.call( "burnCdImage(KURL)", m_args->url(0) );
    else
      instance.call( "burnCdImage()" );
  }
  else {
    for( int i = 0; i < m_args->count(); i++ ) {
      QByteArray singleUrlData;
      QDataStream ds( singleUrlData, IO_WriteOnly );
      ds << m_args->url(i);
      if( projectCreated )
	instance.call( "addUrl(KURL)", m_args->url(i) );
      else
	instance.call( "openDocument(KURL)", m_args->url(i) );
    }
  }
      
  if( m_args->isSet("copycd") )
    instance.call( "copyCd()" );
  else if( m_args->isSet("copydvd") )
    instance.call( "copyDvd()" );
  else if( m_args->isSet("erasecd") )
    instance.call( "eraseCdrw()" );
  else if( m_args->isSet("formatdvd") )
    instance.call( "formatDvd()" );

  m_args->clear();
}


bool K3bSmartInstanceReuser::reuseInstance( KCmdLineArgs* args )
{
  K3bSmartInstanceReuser r( args );

  DCOPRef instance = r.findInstance();
  if( instance.isNull() ) {
    return false;
  }
  else {
    r.reuseInstance( instance );
    return true;
  }
}

