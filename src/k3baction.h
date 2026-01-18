/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
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
                           KActionCollection* actionCollection = nullptr,
                           const QString& actionName = QString() );
    KToggleAction* createToggleAction( QObject* parent,
                                       const QString& text, const QString& icon, const
                                       QKeySequence& shortcut, QObject* receiver, const char* slot,
                                       KActionCollection* actionCollection = nullptr,
                                       const QString& actionName = QString() );
}

#endif
