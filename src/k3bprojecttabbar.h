/***************************************************************************
                          k3bprojecttabbar.h  -  description
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

#ifndef K3BPROJECTTABBAR_H
#define K3BPROJECTTABBAR_H

#include <qtabbar.h>

class QMouseEvent;
class KAction;
class KActionMenu;

/**
  *@author Sebastian Trueg
  */
class K3bProjectTabBar : public QTabBar
{
Q_OBJECT

 public: 
  K3bProjectTabBar( QWidget* parent = 0, const char* name = 0 );
  ~K3bProjectTabBar();

  void insertAction( KAction* );

 protected:
  void mousePressEvent( QMouseEvent* );

 private:
  KActionMenu* m_projectActionMenu;
};

#endif
