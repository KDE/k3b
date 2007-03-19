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

#include <qframe.h>
#include <qstring.h>
#include <qtoolbutton.h>
#include <qptrlist.h>
#include "k3b_export.h"

class KAction;
class KToggleAction;
class KWidgetAction;
class QGridLayout;
class QPopupMenu;
class QResizeEvent;


/**
 * internal class. Do not use!
 */
class LIBK3B_EXPORT K3bToolBoxButton : public QToolButton
{
  Q_OBJECT

 public:
  K3bToolBoxButton( KAction*, QWidget* parent );
  K3bToolBoxButton( const QString& text, const QString& icon, 
		    const QString& tooltip, const QString& whatsthis,
		    QObject* receiver, const char* slot,
		    QWidget* parent );

 private slots:
  void slotPopupActivated();

 protected:
  void resizeEvent( QResizeEvent* );

 private:
  QPopupMenu* m_popupMenu;
};


class LIBK3B_EXPORT K3bToolBox : public QFrame
{
  Q_OBJECT

 public:
  K3bToolBox( QWidget* parent = 0, const char* name = 0 );
  ~K3bToolBox();

  K3bToolBoxButton* addButton( const QString& text, const QString& icon, 
			       const QString& tooltip = QString::null, const QString& whatsthis = QString::null,
			       QObject* receiver = 0, const char* slot = 0,
			       bool forceTextLabel = false );
  K3bToolBoxButton* addButton( KAction*, bool forceTextLabel = false );
  K3bToolBoxButton* addToggleButton( KToggleAction* );
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
  QGridLayout* m_mainLayout;
  QPtrList<QWidget> m_doNotDeleteWidgets;
};


#endif
