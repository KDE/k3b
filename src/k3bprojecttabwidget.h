/***************************************************************************
                          k3bprojecttabwidget.h  -  description
                             -------------------
    begin                : Wed Dec 12 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BPROJECTTABWIDGET_H
#define K3BPROJECTTABWIDGET_H

#include <qtabwidget.h>


class K3bProjectTabBar;
class KAction;


/**
  *@author Sebastian Trueg
  */
class K3bProjectTabWidget : public QTabWidget
{
  Q_OBJECT

 public: 
  K3bProjectTabWidget( QWidget *parent = 0, const char *name = 0, WFlags = 0 );
  ~K3bProjectTabWidget();

  /**
   * inserts the given action into the popup menu for the tabs
   */
  void insertAction( KAction* );

 private:
  K3bProjectTabBar* m_tabBar;
};

#endif
