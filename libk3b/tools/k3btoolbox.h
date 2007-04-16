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

#ifndef K3B_TOOLBOX_H
#define K3B_TOOLBOX_H

#include <qstring.h>
#include <qtoolbutton.h>
#include <qptrlist.h>

#include <ktoolbar.h>

#include "k3b_export.h"

class KAction;
class KToggleAction;
class KWidgetAction;
class QGridLayout;
class QPopupMenu;
class QResizeEvent;
class QMouseEvent;


class LIBK3B_EXPORT K3bToolBox : public KToolBar
{
  Q_OBJECT

 public:
  K3bToolBox( QWidget* parent = 0, const char* name = 0 );
  ~K3bToolBox();

  KToolBarButton* addButton( const QString& text, const QString& icon, 
			       const QString& tooltip = QString::null, const QString& whatsthis = QString::null,
			       QObject* receiver = 0, const char* slot = 0,
			       bool forceTextLabel = false );
  KToolBarButton* addToggleButton( KToggleAction* );

  KToolBarButton* addButton( KAction* action, bool forceText = false );
  void addWidgetAction( KWidgetAction* );

  /**
   * Be aware that the toolbox will take ownership of the widget
   * and destroy it on destruction. Becasue of this it is not fitted
   * for WidgetActions.
   */
  void addWidget( QWidget* );
  void addLabel( const QString& );
  void addSpacing();
  void addSeparator();
  void addStretch();

  void clear();

 protected:
  void mousePressEvent( QMouseEvent* m );

 private slots:
  void slotContextAboutToShow();

 private:
  void saveSettings();
  void loadSettings();

  class Private;
  Private* d;
};


#endif
