/* 
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include <QObject>
#include <QBitmap>


class QTimerEvent;

/**
 * Helper class to show and hide a widget in a fancy way.
 */
namespace K3b {
class WidgetShowEffect : public QObject
{
    Q_OBJECT

public:
    // FIXME: add an effect direction
    enum Effect {
        Dissolve = 1,
        Slide
    };

    explicit WidgetShowEffect( QWidget* widget, Effect e = Slide );
    ~WidgetShowEffect() override;

    void setEffect( Effect e ) { m_effect = e; }

    /**
     * Using the widget effects the easy way.
     * \returns the WidgetShowEffect instance used to show the widget.
     * Can be used to connect to signals.
     */
    static WidgetShowEffect* showWidget( QWidget* w, Effect );

    /**
     * Using the widget effects the easy way.
     * \returns the WidgetShowEffect instance used to hide the widget.
     * Can be used to connect to signals.
     */
    static WidgetShowEffect* hideWidget( QWidget* w, Effect );

Q_SIGNALS:
    void widgetShown( QWidget* );
    void widgetHidden( QWidget* );

public Q_SLOTS:
    /**
     * \param effectOnly If true WidgetShowEffect will not call QWidget::show().
     *                   This is only useful in case onw uses WidgetShowEffect
     *                   to reimplement QWidget::show(). In that case the caller
     *                   has to take care of showing the widget.
     */
    void show( bool effectOnly = false );

    /**
     * \param effectOnly If true WidgetShowEffect will not call QWidget::hide().
     *                   This is only useful in case onw uses WidgetShowEffect
     *                   to reimplement QWidget::hide(). In that case the caller
     *                   has to take care of hiding the widget by connecting to
     *                   WidgetShowEffect::widgetHidden()
     */
    void hide( bool effectOnly = false );

private:
    void timerEvent( QTimerEvent* ) override;
  
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
}

#endif
