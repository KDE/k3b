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

#ifndef K3B_TOOLBOX_H
#define K3B_TOOLBOX_H

#include <qframe.h>
#include <qstring.h>
#include <qtoolbutton.h>

class KAction;
class KToggleAction;
class QGridLayout;
class QPopupMenu;



class K3bToolBoxButton : public QToolButton
{
  Q_OBJECT

 public:
  K3bToolBoxButton( KAction*, QWidget* parent );

 private slots:
  void slotPopupActivated();

 private:
  QPopupMenu* m_popupMenu;
};


class K3bToolBox : public QFrame
{
  Q_OBJECT

 public:
  K3bToolBox( QWidget* parent = 0, const char* name = 0 );
  ~K3bToolBox();

  void addButton( KAction* );
  void addToggleButton( KToggleAction* );
  void addWidget( QWidget* );
  void addLabel( const QString& );
  void addSpacing();
  void addLineSpacing();
  void addStretch();

 protected:
  QGridLayout* m_mainLayout;
};


#endif
