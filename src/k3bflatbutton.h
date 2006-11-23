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

#ifndef FLATBUTTON_H
#define FLATBUTTON_H

#include <qframe.h>
#include <qcolor.h>
#include <qpixmap.h>

class QEvent;
class QMouseEvent;
class QPainter;
class KAction;


/**
@author Sebastian Trueg
*/
class K3bFlatButton : public QFrame
{
  Q_OBJECT

 public:
  K3bFlatButton( QWidget *parent = 0, const char *name = 0 );
  K3bFlatButton( const QString& text, QWidget *parent = 0, const char *name = 0 );
  K3bFlatButton( KAction*, QWidget *parent = 0, const char *name = 0 );
  
  ~K3bFlatButton();

  QSize sizeHint() const;

 public slots:
  void setColors( const QColor& fore, const QColor& back );
  void setText( const QString& );
  void setPixmap( const QPixmap& );

 signals:
  void pressed();
  void clicked();

 private slots:
  void slotThemeChanged();

 private:
  void init();

  void mousePressEvent(QMouseEvent* e);
  void mouseReleaseEvent(QMouseEvent* e);
  void enterEvent( QEvent* );
  void leaveEvent( QEvent* );
  void drawContents( QPainter* );

  void setHover( bool );

  bool m_pressed;
  QColor m_backColor;
  QColor m_foreColor;
  QString m_text;
  QPixmap m_pixmap;

  bool m_hover;
};

#endif
