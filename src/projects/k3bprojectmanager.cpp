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

#include "k3bprojectmanager.h"

#include <k3bdoc.h>

#include <qptrlist.h>

#include <kurl.h>
#include <kdebug.h>


class K3bProjectManager::Private
{
public:
  QPtrList<K3bDoc> projects;
  K3bDoc* activeProject;
};


K3bProjectManager* K3bProjectManager::s_k3bProjectManager = 0;


K3bProjectManager::K3bProjectManager( QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private();
  d->activeProject = 0;

  if( s_k3bProjectManager ) {
    qFatal("ONLY ONE INSTANCE OF K3BPROJECTMANAGER ALLOWED!");
  }

  s_k3bProjectManager = this;
}

K3bProjectManager::~K3bProjectManager()
{
  delete d;
}


const QPtrList<K3bDoc>& K3bProjectManager::projects() const
{
  return d->projects;
}


void K3bProjectManager::addProject( K3bDoc* doc )
{
  kdDebug() << "(K3bProjectManager) adding doc " << doc->URL().path() << endl;

  d->projects.append(doc);
  emit newProject( doc );
}


void K3bProjectManager::removeProject( K3bDoc* doc )
{
  // 
  // QPtrList.findRef seems to be buggy. Everytime we search for the 
  // first added item it is not found!
  //
  for( QPtrListIterator<K3bDoc> it( d->projects );
       it.current(); ++it ) {
    if( it.current() == doc ) {
      d->projects.removeRef(doc);
      emit closingProject(doc);
      
      return;
    }
  }
  kdDebug() << "(K3bProjectManager) unable to find doc: " << doc->URL().path() << endl;
}


K3bDoc* K3bProjectManager::findByUrl( const KURL& url )
{
  for( QPtrListIterator<K3bDoc> it( d->projects );
       it.current(); ++it ) {
    K3bDoc* doc = it.current();
    if( doc->URL() == url )
      return doc;
  }
  return 0;
}


bool K3bProjectManager::isEmpty() const
{
  return d->projects.isEmpty();
}


void K3bProjectManager::setActive( K3bDoc* doc )
{
  // 
  // QPtrList.findRef seems to be buggy. Everytime we search for the 
  // first added item it is not found!
  //
  for( QPtrListIterator<K3bDoc> it( d->projects );
       it.current(); ++it ) {
    if( it.current() == doc ) {
      d->activeProject = doc;
      emit activeProjectChanged(doc);
    }
  }
}


K3bDoc* K3bProjectManager::activeDoc() const
{
  return d->activeProject;
}

#include "k3bprojectmanager.moc"
