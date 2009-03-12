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

#ifndef _K3B_JOB_PROGRESS_OSD_H_
#define _K3B_JOB_PROGRESS_OSD_H_

#include <qwidget.h>
#include <qpixmap.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QMouseEvent>

class QPaintEvent;
class QMouseEvent;
class KConfigGroup;

namespace K3b {
    /**
     * An OSD displaying a text and a progress bar.
     *
     * Insprired by Amarok's OSD (I also took a bit of their code. :)
     */
    class JobProgressOSD : public QWidget
    {
        Q_OBJECT

    public:
        JobProgressOSD( QWidget* parent = 0 );
        ~JobProgressOSD();

        int screen() const { return m_screen; }
        QPoint position() const { return m_position; }

        void readSettings( const KConfigGroup& );
        void saveSettings( KConfigGroup );

    public Q_SLOTS:
        void setScreen( int );
        void setText( const QString& );
        void setProgress( int );

        /**
         * The position refers to one of the corners of the widget
         * regarding on the value of the x and y coordinate.
         * If for example the x coordinate is bigger than half the screen
         * width it refers to the left edge of the widget.
         */
        void setPosition( const QPoint& );

        void show();

    protected:
        void paintEvent( QPaintEvent* );
        void mousePressEvent( QMouseEvent* );
        void mouseReleaseEvent( QMouseEvent* );
        void mouseMoveEvent( QMouseEvent* );
        void reposition();

    private:
        /**
         * Ensure that the position is inside m_screen
         */
        QPoint fixupPosition( const QPoint& p );
        static const int s_outerMargin = 15;

        QString m_text;
        int m_progress;
        bool m_dragging;
        QPoint m_dragOffset;
        int m_screen;
        QPoint m_position;
    };
}

#endif
