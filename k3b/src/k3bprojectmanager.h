/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_PROJECT_MANAGER_H_
#define _K3B_PROJECT_MANAGER_H_

#include <qobject.h>
#include <qptrlist.h>
#include <k3bdoc.h>


class KURL;
class K3bProjectInterface;


class K3bProjectManager : public QObject
{
  Q_OBJECT

 public:
  K3bProjectManager( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bProjectManager();

  const QPtrList<K3bDoc>& projects() const;

  /**
   * Create a new project including loading user defaults and creating
   * the dcop interface.
   */
  K3bDoc* createProject( K3bDoc::DocType type );

  /**
   * Opens a K3b project.
   * \return 0 if url does not point to a valid k3b project file, the new project otherwise.
   */
  K3bDoc* openProject( const KURL &url );

  /**
   * saves the document under filename and format.
   */
  bool saveProject( K3bDoc*, const KURL &url );

  K3bDoc* activeDoc() const { return activeProject(); }
  K3bDoc* activeProject() const;
  K3bDoc* findByUrl( const KURL& url );
  bool isEmpty() const;

  /**
   * Will create if none exists.
   */
  K3bProjectInterface* dcopInterface( K3bDoc* doc );

 public slots:
  void addProject( K3bDoc* );
  void removeProject( K3bDoc* );
  void setActive( K3bDoc* );
  void loadDefaults( K3bDoc* );

 signals:
  void newProject( K3bDoc* );
  void projectSaved( K3bDoc* );
  void closingProject( K3bDoc* );
  void projectChanged( K3bDoc* doc );
  void activeProjectChanged( K3bDoc* );

 private slots:
  void slotProjectChanged( K3bDoc* doc );

 private:
  // used internal
  K3bDoc* createEmptyProject( K3bDoc::DocType );

  class Private;
  Private* d;
};

#endif
