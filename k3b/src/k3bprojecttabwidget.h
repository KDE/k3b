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


#ifndef K3BPROJECTTABWIDGET_H
#define K3BPROJECTTABWIDGET_H

#include <qtabwidget.h>
#include <kurl.h>

class KAction;
class KActionMenu;
class K3bDoc;


/**
 * An enhanced Tab Widget that hides the tabbar in case only one page has been inserted
 * and shows a context menu fpr K3b projects.
 *
 * @author Sebastian Trueg
 */
class K3bProjectTabWidget : public QTabWidget
{
  Q_OBJECT

 public: 
  K3bProjectTabWidget( QWidget *parent = 0, const char *name = 0, WFlags = 0 );
  ~K3bProjectTabWidget();

  void insertTab( K3bDoc* );
  
  void addTab( QWidget * child, const QString & label );
  void addTab( QWidget * child, const QIconSet & iconset, const QString & label );
  void addTab( QWidget * child, QTab * tab );
  void insertTab( QWidget * child, const QString & label, int index = -1 );
  void insertTab( QWidget * child, const QIconSet & iconset, const QString & label, int index = -1 );
  void insertTab( QWidget * child, QTab * tab, int index = -1 );

  /**
   * \return the project for the tab at position \p pos or 0 in case the tab is
   * not a project tab.
   */
  K3bDoc* projectAt( const QPoint& pos ) const;

  /**
   * inserts the given action into the popup menu for the tabs
   */
  void insertAction( KAction* );

  bool eventFilter( QObject* o, QEvent* e );

 public slots:
  void removePage( QWidget* );

 private slots:
  void slotDocChanged( K3bDoc* );
  void slotDocSaved( K3bDoc* );

 private:
  KActionMenu* m_projectActionMenu;

  class ProjectData;
  QMap<K3bDoc*, ProjectData> m_projectDataMap;
};

#endif
