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

#ifndef _K3B_PROJECT_MANAGER_H_
#define _K3B_PROJECT_MANAGER_H_

#include <qobject.h>
#include <qptrlist.h>

class K3bDoc;
class KURL;



#define k3bprojectmanager K3bProjectManager::instance()


/**
 * The K3bProjectManager is the core of the project lib.
 * It may be compared to K3bCore, the core of the core lib. ;)
 * It is mainly used to allow access to all currently open 
 * projects. You ALWAYS need an instance when using K3bDocs
 * since K3bDoc calls the add and remove methods
 */
class K3bProjectManager : public QObject
{
  Q_OBJECT

 public:
  K3bProjectManager( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bProjectManager();

  const QPtrList<K3bDoc>& projects() const;

  K3bDoc* activeDoc() const;
  K3bDoc* findByUrl( const KURL& url );
  bool isEmpty() const;

  static K3bProjectManager* instance() { return s_k3bProjectManager; }

 public slots:
  /**
   * Called by K3bDoc. No need to add projects manually.
   */
  void addProject( K3bDoc* );

  /**
   * Called by K3bDoc. No need to remove projects manually.
   */
  void removeProject( K3bDoc* );
  void setActive( K3bDoc* );

 signals:
  void newProject( K3bDoc* );
  void closingProject( K3bDoc* );
  void activeProjectChanged( K3bDoc* );

 private:
  class Private;
  Private* d;

  static K3bProjectManager* s_k3bProjectManager;
};

#endif
