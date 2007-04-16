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

#ifndef _K3B_WIDGETFACTORY_ACTION_H_
#define _K3B_WIDGETFACTORY_ACTION_H_

#include <kaction.h>

/**
 * A K3bWidgetFactoryAction behaves just like a KWidgetAction except that
 * for each plug operation a new widget is created. Thus, it can be plugged into
 * multiple containers at the same time.
 */
class K3bWidgetFactoryAction : public KAction
{
    Q_OBJECT

 public:
    K3bWidgetFactoryAction( QObject* parent = 0, const char* name = 0 );
    ~K3bWidgetFactoryAction();

    virtual int plug( QWidget* widget, int index = -1 );
    virtual void unplug( QWidget* w );

    QWidget* widget( QWidget* container );

 protected:
    /**
     * Create a new widget to be plugged into a container.
     */
    virtual QWidget* createWidget( QWidget* container ) = 0;

 private slots:
    void slotDestroyed();

 private:
    class Private;
    Private* d;
};

#endif
