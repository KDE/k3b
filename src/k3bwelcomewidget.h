/* 
 *
 * $Id$
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


#ifndef _K3B_WELCOME_WIDGET_H_
#define _K3B_WELCOME_WIDGET_H_

#include <qscrollview.h>

#include <kurl.h>

class K3bMainWindow;
class QDropEvent;
class QDragEnterEvent;
class QToolButton;
class QPaintEvent;
class QResizeEvent;
class QSimpleRichText;


class K3bWelcomeWidget : public QScrollView
{
  Q_OBJECT

 public:
  K3bWelcomeWidget( K3bMainWindow*, QWidget* parent = 0, const char* name = 0 );
  ~K3bWelcomeWidget();

  class Display;

 protected:
  void resizeEvent( QResizeEvent* );

 private:
  K3bMainWindow* m_mainWindow;
  Display* main;
};


class K3bWelcomeWidget::Display : public QWidget
{
  Q_OBJECT

 public:
  Display( QWidget* parent );

  QToolButton* audioDocButton;
  QToolButton* dataDocButton;
  QToolButton* dataDvdDocButton;
  QToolButton* copyCdButton;

  QSize sizeHint() const { return m_size; }

 signals:
  void dropped( const KURL::List& );

 protected:
  void paintEvent( QPaintEvent* );
  void dropEvent( QDropEvent* event );
  void dragEnterEvent( QDragEnterEvent* event );

 private:
  QSimpleRichText* m_header;
  QSize m_size;
};


#endif
