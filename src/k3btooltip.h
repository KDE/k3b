/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_TOOLTIP_H_
#define _K3B_TOOLTIP_H_

#include <qobject.h>

class QTimer;

/**
 * More beautiful tooltip
 */
class K3bToolTip : public QObject
{
  Q_OBJECT

 public:
  K3bToolTip( QWidget* widget );
  ~K3bToolTip();

  QWidget* parentWidget() const { return m_parentWidget; }

 public slots:
  /**
   * default is 2 seconds.
   */
  void setTipTimeout( int msec ) { m_tipTimeout = msec; }

 protected:
  /**
   * \see QToolTip::maybeTip
   */
  virtual void maybeTip( const QPoint& ) = 0;

  /**
   * Show a tooltip.
   */
  void tip( const QRect&, const QString& );

  /**
   * Use some arbitrary widget as the tooltip
   */
  void tip( const QRect&, QWidget* w );

  bool eventFilter( QObject* o, QEvent* e );

 private slots:
  void slotCheckShowTip();

 private:
  void hideTip();

  QWidget* m_parentWidget;
  QWidget* m_currentTip;
  QRect m_currentTipRect;

  QTimer* m_tipTimer;
  QPoint m_lastMousePos;

  int m_tipTimeout;
};

#endif
