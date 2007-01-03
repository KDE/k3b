/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * Based on the effects in popupMessage.cpp 
 * Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>
 *               2005 by Seb Ruiz <me@sebruiz.net>
 *
 * Dissolve Mask (c) Kicker Authors kickertip.cpp, 2005/08/17
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_WIDGET_SHOW_EFFECT_H_
#define _K3B_WIDGET_SHOW_EFFECT_H_

#include <qobject.h>
#include <qbitmap.h>


/**
 * Helper class to show and hide a widget in a fancy way.
 */
class K3bWidgetShowEffect : public QObject
{
  Q_OBJECT

 public:
  // FIXME: add an effect direction
  enum Effect {
    Dissolve = 1,
    Slide
  };

  K3bWidgetShowEffect( QWidget* widget, Effect e = Slide );
  ~K3bWidgetShowEffect();

  void setEffect( Effect e ) { m_effect = e; }

  /**
   * Using the widget effects the easy way.
   * \returns the K3bWidgetShowEffect instance used to show the widget.
   * Can be used to connect to signals.
   */
  static K3bWidgetShowEffect* showWidget( QWidget* w, Effect );

  /**
   * Using the widget effects the easy way.
   * \returns the K3bWidgetShowEffect instance used to hide the widget.
   * Can be used to connect to signals.
   */
  static K3bWidgetShowEffect* hideWidget( QWidget* w, Effect );

 signals:
  void widgetShown( QWidget* );
  void widgetHidden( QWidget* );

 public slots:
  /**
   * \param effectOnly If true K3bWidgetShowEffect will not call QWidget::show().
   *                   This is only useful in case onw uses K3bWidgetShowEffect
   *                   to reimplement QWidget::show(). In that case the caller
   *                   has to take care of showing the widget.
   */
  void show( bool effectOnly = false );

  /**
   * \param effectOnly If true K3bWidgetShowEffect will not call QWidget::hide().
   *                   This is only useful in case onw uses K3bWidgetShowEffect
   *                   to reimplement QWidget::hide(). In that case the caller
   *                   has to take care of hiding the widget by connecting to
   *                   K3bWidgetShowEffect::widgetHidden()
   */
  void hide( bool effectOnly = false );

 private:
  void timerEvent( QTimerEvent* );
  
  /**
   * @short Gradually show widget by dissolving from background
   */
  void dissolveMask();
  
  /**
   * @short animation to slide the widget into view
   */
  void slideMask();

  Effect m_effect;
  QWidget* m_widget;

  QBitmap m_mask;

  int m_dissolveSize;
  int m_dissolveDelta;

  int m_offset;
  int m_timerId;

  // if true we show, otherwise we hide the widget
  bool m_bShow;

  bool m_deleteSelf;
  bool m_bEffectOnly;
};

#endif
