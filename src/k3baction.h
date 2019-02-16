/*
 *
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_ACTION_H_
#define _K3B_ACTION_H_

#include <KToggleAction>
#include <KActionCollection>

#include <QObject>
#include <QIcon>
#include <QAction>

namespace K3b {
    /**
     * Create a QAction the old (way cooler) KDE3 way. Porting becomes a simple replacement.
     */
    QAction* createAction( QObject* parent,
                           const QString& text, const QString& icon, const
                           QKeySequence& shortcut, QObject* receiver, const char* slot,
                           KActionCollection* actionCollection = 0,
                           const QString& actionName = QString() );
    KToggleAction* createToggleAction( QObject* parent,
                                       const QString& text, const QString& icon, const
                                       QKeySequence& shortcut, QObject* receiver, const char* slot,
                                       KActionCollection* actionCollection = 0,
                                       const QString& actionName = QString() );
}

#endif
