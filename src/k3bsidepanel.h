/* 
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_SIDE_PANEL_H_
#define _K3B_SIDE_PANEL_H_

#include <qtoolbox.h>
//Added by qt3to4:
#include <QFrame>

class K3bMainWindow;
class K3bFileTreeView;
class QFrame;
class KAction;


class K3bSidePanel : public QToolBox
{
  Q_OBJECT

 public:
  K3bSidePanel( K3bMainWindow*, QWidget* parent = 0 );
  ~K3bSidePanel();

  /**
   * This should be removed in the future. For now we need it because of the
   * bad design of the dirview. :(
   */
  K3bFileTreeView* fileTreeView() const { return m_fileTreeView; }

 private:
  K3bMainWindow* m_mainWindow;
  K3bFileTreeView* m_fileTreeView;

  QFrame* createPanel();
  void addButton( QFrame* frame, QAction* action );
};

#endif
