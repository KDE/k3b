/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_PROJECT_MANAGER_H_
#define _K3B_PROJECT_MANAGER_H_

#include <qobject.h>
#include <qptrlist.h>

class K3bDoc;
class KURL;


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

 public slots:
  void addProject( K3bDoc* );
  void removeProject( K3bDoc* );
  void setActive( K3bDoc* );

 signals:
  void newProject( K3bDoc* );
  void closingProject( K3bDoc* );
  void activeProjectChanged( K3bDoc* );

 private:
  class Private;
  Private* d;
};

#endif
