/*
 *
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3BSETUPWORKER_H_
#define _K3BSETUPWORKER_H_
 
#include <QDBusContext>
#include <QObject>
#include <QStringList>
#include <QVariantList>
 
namespace K3b {
namespace Setup {
 
class Worker : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.k3b.setup")

public:
	Worker( QObject *parent = 0 );
    virtual ~Worker();

public Q_SLOTS:
    void updatePermissions( QString burningGroup, QStringList devices, QVariantList programs );

Q_SIGNALS:
    void done( QStringList updated, QStringList failedToUpdate );
    void authorizationFailed();
    
};

} // namespace Setup
} // namespace K3b
 
#endif
