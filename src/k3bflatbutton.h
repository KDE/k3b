/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FLATBUTTON_H
#define FLATBUTTON_H

#include <QColor>
#include <QAbstractButton>

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
        explicit FlatButton( QWidget* parent = nullptr );
        explicit FlatButton( const QString& text, QWidget* parent = nullptr );
        explicit FlatButton( QAction* action, QWidget* parent = nullptr );
    
        ~FlatButton() override;

        QSize sizeHint() const override;

    public Q_SLOTS:
        void setColors( const QColor& fore, const QColor& back );

    private Q_SLOTS:
        void slotThemeChanged();

    private:
        void init();
        bool event( QEvent* event ) override;
        void enterEvent( QEnterEvent* event ) override;
        void leaveEvent( QEvent* event ) override;
        void paintEvent( QPaintEvent* event ) override;
        void setHover( bool );

        QColor m_backColor;
        QColor m_foreColor;

        bool m_hover;
    };
}

#endif
