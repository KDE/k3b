/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bpushbutton.h"

#include <qtimer.h>
#include <qmenu.h>
#include <qevent.h>

#include <KDELibs4Support/KDE/KGlobalSettings>
#include <kapplication.h>

#include <QtWidgets/QDesktopWidget>


class K3b::PushButton::Private
{
public:
    Private()
        : popupTimer(0) {
    }

    QTimer* popupTimer;
    QPoint mousePressPos;
};



K3b::PushButton::PushButton( QWidget* parent )
    : KPushButton( parent )
{
    d = new Private();
    installEventFilter(this);
}


K3b::PushButton::PushButton( const QString& text, QWidget* parent )
    : KPushButton( text, parent )
{
    d = new Private();
    installEventFilter(this);
}


// K3b::PushButton::PushButton( const QIcon& icon, const QString& text,
// 			      QWidget* parent )
//   : KPushButton( icon, text, parent )
// {
//   d = new Private();
//   installEventFilter(this);
// }


K3b::PushButton::PushButton( const KGuiItem& item, QWidget* parent )
    : KPushButton( item, parent )
{
    d = new Private();
    installEventFilter(this);
}


K3b::PushButton::~PushButton()
{
    delete d;
}


void K3b::PushButton::setDelayedPopupMenu( QMenu* menu )
{
    if( !d->popupTimer ) {
        d->popupTimer = new QTimer( this );
        connect( d->popupTimer, SIGNAL(timeout()), this, SLOT(slotDelayedPopup()) );
    }

    setMenu( menu );

    // we need to do the menu handling ourselves so we cheat a little
    // QPushButton connects a menu slot to the pressed signal which we disconnect here
    disconnect( this );
}


bool K3b::PushButton::eventFilter( QObject* o, QEvent* ev )
{
    if( dynamic_cast<K3b::PushButton*>(o) == this ) {

        // Popup the menu when the left mousebutton is pressed and the mouse
        // is moved by a small distance.
        if( menu() ) {
            if( ev->type() == QEvent::MouseButtonPress ) {
                QMouseEvent* mev = static_cast<QMouseEvent*>(ev);
                d->mousePressPos = mev->pos();
                d->popupTimer->start( QApplication::startDragTime() );
            }
            else if( ev->type() == QEvent::MouseMove ) {
                QMouseEvent* mev = static_cast<QMouseEvent*>(ev);
                if( ( mev->pos() - d->mousePressPos).manhattanLength() > KGlobalSettings::dndEventDelay() ) {
                    d->popupTimer->stop();
                    slotDelayedPopup();
                    return true;
                }
            }
        }
    }

    return KPushButton::eventFilter( o, ev );
}


void K3b::PushButton::slotDelayedPopup()
{
    d->popupTimer->stop();

    if( isDown() ) {
        // popup the menu.
        // this has been taken from the QPushButton code
        if( mapToGlobal( QPoint( 0, rect().bottom() ) ).y() + menu()->sizeHint().height() <= qApp->desktop()->height() )
            menu()->exec( mapToGlobal( rect().bottomLeft() ) );
        else
            menu()->exec( mapToGlobal( rect().topLeft() - QPoint( 0, menu()->sizeHint().height() ) ) );
        setDown( false );
    }
}

#include "k3bpushbutton.moc"
