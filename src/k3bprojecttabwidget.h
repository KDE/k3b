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


#ifndef K3BPROJECTTABWIDGET_H
#define K3BPROJECTTABWIDGET_H

#include <qtabwidget.h>
#include <kurl.h>

class K3bProjectTabBar;
class KAction;
class K3bDoc;


/**
  *@author Sebastian Trueg
  */
class K3bProjectTabWidget : public QTabWidget
{
  Q_OBJECT

 public: 
  K3bProjectTabWidget( QWidget *parent = 0, const char *name = 0, WFlags = 0 );
  ~K3bProjectTabWidget();

  void insertTab( K3bDoc* );

  /**
   * inserts the given action into the popup menu for the tabs
   */
  void insertAction( KAction* );

 private slots:
  void slotUrlsDropped( int id, const KURL::List& ulrs );
  void slotDocChanged( K3bDoc* );
  void slotDocSaved( K3bDoc* );

 private:
  K3bProjectTabBar* m_tabBar;
};

#endif
