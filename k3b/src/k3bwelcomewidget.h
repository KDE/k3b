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


#ifndef _K3B_WELCOME_WIDGET_H_
#define _K3B_WELCOME_WIDGET_H_

#include <qscrollview.h>
#include <qptrlist.h>
#include <qmap.h>

#include <kurl.h>
#include <kaction.h>

class K3bMainWindow;
class QDropEvent;
class QDragEnterEvent;
class K3bFlatButton;
class QPaintEvent;
class QResizeEvent;
class QSimpleRichText;
class KConfigBase;
class QMouseEvent;
class QShowEvent;


class K3bWelcomeWidget : public QScrollView
{
  Q_OBJECT

 public:
  K3bWelcomeWidget( K3bMainWindow*, QWidget* parent = 0, const char* name = 0 );
  ~K3bWelcomeWidget();

  void loadConfig( KConfigBase* c );
  void saveConfig( KConfigBase* c );

  class Display;

 protected:
  void resizeEvent( QResizeEvent* );
  void showEvent( QShowEvent* );
  void contentsMousePressEvent( QMouseEvent* e );

 private:
  void fixSize();

  K3bMainWindow* m_mainWindow;
  Display* main;
};


class K3bWelcomeWidget::Display : public QWidget
{
  Q_OBJECT

 public:
  Display( QWidget* parent );
  ~Display();

  QSize minimumSizeHint() const;
  QSizePolicy sizePolicy () const;
  int heightForWidth ( int w ) const;

  void addAction( KAction* );
  void removeAction( KAction* );
  void removeButton( K3bFlatButton* );
  void rebuildGui();
  void rebuildGui( const QPtrList<KAction>& );

 signals:
  void dropped( const KURL::List& );

 protected:
  void resizeEvent( QResizeEvent* );
  void paintEvent( QPaintEvent* );
  void dropEvent( QDropEvent* event );
  void dragEnterEvent( QDragEnterEvent* event );

 private:
  void repositionButtons();

  QSimpleRichText* m_header;
  QSimpleRichText* m_infoText;

  QSize m_buttonSize;
  int m_cols;
  int m_rows;

  QPtrList<KAction> m_actions;
  QPtrList<K3bFlatButton> m_buttons;
  QMap<K3bFlatButton*, KAction*> m_buttonMap;

  bool m_infoTextVisible;

  friend class K3bWelcomeWidget;
};

#endif
