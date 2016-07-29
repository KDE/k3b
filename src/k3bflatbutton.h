/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef FLATBUTTON_H
#define FLATBUTTON_H

#include <QAbstractButton>
#include <QColor>

class QEvent;
class QMouseEvent;
class QAction;
class QPaintEvent;

/**
   @author Sebastian Trueg
*/
namespace K3b {
    class FlatButton : public QAbstractButton
    {
        Q_OBJECT

    public:
        FlatButton( QWidget* parent = 0 );
        FlatButton( const QString& text, QWidget* parent = 0 );
        FlatButton( QAction* action, QWidget* parent = 0 );
    
        ~FlatButton();

        virtual QSize sizeHint() const;

    public Q_SLOTS:
        void setColors( const QColor& fore, const QColor& back );

    private Q_SLOTS:
        void slotThemeChanged();

    private:
        void init();

        virtual void enterEvent( QEvent* event );
        virtual void leaveEvent( QEvent* event );
        virtual void paintEvent( QPaintEvent* event );
        void setHover( bool );

        QColor m_backColor;
        QColor m_foreColor;

        bool m_hover;
    };
}

#endif
