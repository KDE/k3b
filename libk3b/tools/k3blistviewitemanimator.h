/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_LISTVIEWITEM_ANIMATOR_H_
#define _K3B_LISTVIEWITEM_ANIMATOR_H_

#include <qobject.h>
#include <qpixmap.h>


class QListViewItem;
class QTimer;


/**
 * Fades an icon on a listview item in and out.
 */
class K3bListViewItemAnimator : public QObject
{
  Q_OBJECT

 public:
  K3bListViewItemAnimator( QObject* parent = 0, const char* name = 0 );
  /**
   * Will use the items pixmap.
   */
  K3bListViewItemAnimator( QListViewItem* item, int col, QObject* parent = 0, const char* name = 0 );
  ~K3bListViewItemAnimator();

 public slots:
  void start();
  void stop();

  void setItem( QListViewItem*, int col );

  /**
   * Default is the pixmap from the item.
   */
  void setPixmap( const QPixmap& );

  void setColumn( int col );

  /**
   * Default is the base color of the listview.
   */
  void setFadeColor( const QColor& );

 private slots:
  void slotAnimate();

 private:
 void init();

  int m_animationStep;
  bool m_animationBack;
  QPixmap m_pixmap;
  QColor m_fadeColor;
  QListViewItem* m_item;
  int m_column;

  QTimer* m_timer;
};

#endif
