/* 
 *
 * $Id: $
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
