/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
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
#include <qpixmap.h>
//Added by qt3to4:
#include <QEvent>

#include "k3bwidgetshoweffect.h"

class QTimer;

namespace K3b {
    /**
     * More beautiful tooltip
     */
    class ToolTip : public QObject
    {
        Q_OBJECT

    public:
        ToolTip( QWidget* widget );
        ~ToolTip();

        QWidget* parentWidget() const { return m_parentWidget; }

    public Q_SLOTS:
        /**
         * default is 700 mseconds (same as QToolTip)
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
        void tip( const QRect&, const QString&, int effect = WidgetShowEffect::Dissolve );
        void tip( const QRect& rect, const QPixmap& pix, int effect = WidgetShowEffect::Dissolve );

        /**
         * Use some arbitrary widget as the tooltip
         * \param effect Use 0 for no effect
         */
        void tip( const QRect&, QWidget* w, int effect = WidgetShowEffect::Dissolve );

        bool eventFilter( QObject* o, QEvent* e );

    private Q_SLOTS:
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
}

#endif
