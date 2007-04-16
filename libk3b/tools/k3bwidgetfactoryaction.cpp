/*
 *
 * $Id: k3baudiotrackplayer.cpp 619556 2007-01-03 17:38:12Z trueg $
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bwidgetfactoryaction.h"

#include <ktoolbar.h>
#include <kdebug.h>


K3bWidgetFactoryAction::K3bWidgetFactoryAction( QObject* parent, const char* name )
    : KAction( parent, name )
{
}


K3bWidgetFactoryAction::~K3bWidgetFactoryAction()
{
}


QWidget* K3bWidgetFactoryAction::widget( QWidget* container )
{
    int index = findContainer( container );
    if ( index != -1 ) {
        if ( ::qt_cast<KToolBar*>( container ) ) {
            KToolBar* toolBar = static_cast<KToolBar*>( container );
            return toolBar->getWidget( itemId( index ) );
        }
        else {
            kdDebug() << "(K3bWidgetFactoryAction) container is no KToolBar: " << container << endl;
        }
    }
    else {
        kdDebug() << "(K3bWidgetFactoryAction) could not find container " << container << endl;
    }

    return 0;
}


int K3bWidgetFactoryAction::plug( QWidget* container, int index )
{
    // FIMXE: handle menus
    if ( ::qt_cast<KToolBar *>( container ) ) {
        KToolBar* toolBar = static_cast<KToolBar*>( container );

        QWidget* widget = createWidget( container );

        int id = getToolButtonID();
        toolBar->insertWidget( id, widget->sizeHint().width(), widget, index );

        addContainer( container, id );
        connect( toolBar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

        kdDebug() << "(K3bWidgetFactoryAction) added widget " << widget << " for container " << container << " with id " << id << endl;

        return containerCount() - 1;
    }

    return -1;
}


void K3bWidgetFactoryAction::unplug( QWidget* w )
{
    int id = findContainer( w );
    if ( id != -1 ) {
        if ( ::qt_cast<KToolBar *>( w ) ) {
            KToolBar *bar = static_cast<KToolBar *>( w );
            bar->removeItemDelayed( id );
        }
        removeContainer( id );
    }
}


void K3bWidgetFactoryAction::slotDestroyed()
{
    const QObject* o = sender();
    int i = -1;
    while ( ( i = findContainer( static_cast<const QWidget*>( o ) ) ) != -1 ) {
        removeContainer( i );
    }
}

#include "k3bwidgetfactoryaction.moc"
