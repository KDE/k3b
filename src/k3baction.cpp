/*
 *
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baction.h"

QAction* K3b::createAction( QObject* parent,
                            const QString& text, const QString& icon, const
                            QKeySequence& shortcut, QObject* receiver, const char* slot,
                            KActionCollection* actionCollection,
                            const QString& actionName )
{
    QAction* action = new QAction( parent );
    action->setText( text );
    if( !icon.isEmpty() ) {
        action->setIcon( QIcon::fromTheme( icon ) );
    }
    action->setShortcut( shortcut );
    if( receiver ) {
        QObject::connect( action, SIGNAL(triggered()),
                          receiver, slot );
    }
    if( actionCollection ) {
        actionCollection->addAction( actionName, action );
    }
    return action;
}


KToggleAction* K3b::createToggleAction( QObject* parent,
                                        const QString& text, const QString& icon, const
                                        QKeySequence& shortcut, QObject* receiver, const char* slot,
                                        KActionCollection* actionCollection,
                                        const QString& actionName )
{
    KToggleAction* action = new KToggleAction( parent );
    action->setText( text );
    if( !icon.isEmpty() ) {
        action->setIcon( QIcon::fromTheme( icon ) );
    }
    action->setShortcut( shortcut );
    if( receiver ) {
        QObject::connect( action, SIGNAL(triggered(bool)),
                          receiver, slot );
    }
    if( actionCollection ) {
        actionCollection->addAction( actionName, action );
    }
    return action;
}
