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
#include <kurl.h>
#include <kwin.h>
#include <kstartupinfo.h>

// arghhh.. why is saveAppArgs private??? Why why why
#define private public
#include <kcmdlineargs.h>
#undef private

#include <qcstring.h>


K3bSmartInstanceReuser::K3bSmartInstanceReuser( KCmdLineArgs* args )
  : DCOPClient(),
    m_args( args )
{
  registerAs( "smart_k3b_instancer" );
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

  // raise the found instance to the foreground
  QCString new_asn_id;
  KStartupInfoId id = KStartupInfo::currentStartupIdEnv();
  if( !id.none())
    new_asn_id = id.id();
  QByteArray data, reply;
  QCString replyType;
  QDataStream ds(data, IO_WriteOnly);
  KCmdLineArgs::saveAppArgs(ds);
  ds << new_asn_id;

  DCOPClient dc;
  if( !dc.attach() ) {
    kdError() << "(K3bSmartInstanceReuser) cannot attach to dcop server." << endl;
    return;
  }

  // KUniqueApplication does this...
  dc.setPriorityCall(true);
  if( !dc.call( instance.app(), "K3b", "reuseInstance()", data, replyType, reply ) ) {
    kdError() << "(K3bSmartInstanceReuser) comunication error with " << instance.app() << endl;
    return;
  }
  dc.setPriorityCall(false);

  return;
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

