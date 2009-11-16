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

#include "k3bsetupworker.h"
#include "setupadaptor.h"
#include "k3bsetupprogramitem.h"

#include <QString>
#include <polkit-qt/Auth>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>

namespace {

bool updateDevicePermissions( struct group* g, const QString& device )
{
    bool success = true;
    if( g != 0 ) {
        if( ::chmod( QFile::encodeName(device), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP ) )
            success = false;

        if( ::chown( QFile::encodeName(device), (gid_t)-1, g->gr_gid ) )
            success = false;
    }
    else {
        if( ::chmod( QFile::encodeName(device), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH ) )
            success = false;
    }
    return success;
}


bool updateProgramPermissions( struct group* g, const QString& path, bool suid )
{
    bool success = true;
    if( g != 0 ) {
        if( ::chown( QFile::encodeName(path), (gid_t)0, g->gr_gid ) )
            success = false;

        int perm = 0;
        if( suid )
            perm = S_ISUID|S_IRWXU|S_IXGRP;
        else
            perm = S_IRWXU|S_IXGRP|S_IRGRP;

        if( ::chmod( QFile::encodeName(path), perm ) )
            success = false;
    }
    else {
        if( ::chown( QFile::encodeName(path), 0, 0 ) )
            success = false;

        int perm = 0;
        if( suid )
            perm = S_ISUID|S_IRWXU|S_IXGRP|S_IXOTH;
        else
            perm = S_IRWXU|S_IXGRP|S_IRGRP|S_IXOTH|S_IROTH;

        if( ::chmod( QFile::encodeName(path), perm ) )
            success = false;
    }
    return success;
}

} // namespace


namespace K3b {
namespace Setup {

Worker::Worker( QObject* parent )
:
    QObject( parent )
{
    new SetupAdaptor(this);

    if( !QDBusConnection::systemBus().registerService( "org.k3b.setup" ) ) {
        qDebug() << "another helper is already running";
        QTimer::singleShot( 0, QCoreApplication::instance(), SLOT(quit()) );
        return;
    }

    if( !QDBusConnection::systemBus().registerObject( "/", this ) ) {
        qDebug() << "unable to register service interface to dbus";
        QTimer::singleShot( 0, QCoreApplication::instance(), SLOT(quit()) );
        return;
    }
}


Worker::~Worker()
{
}


void Worker::updatePermissions( QString burningGroup, QStringList devices, QVariantList programs )
{
    // Check whether this action has been authorized before being called
    PolkitQt::Auth::Result result = PolkitQt::Auth::isCallerAuthorized(
            "org.k3b.setup.update-permissions",
            message().service(),
            true );

    if( result == PolkitQt::Auth::Yes ) {
        qDebug() << message().service() << QString(" authorized");
        
        struct group* g = 0;
        if( !burningGroup.isEmpty() ) {
            // TODO: create the group if it's not there
            g = getgrnam( burningGroup.toLocal8Bit() );
        }
        
        QStringList updated;
        QStringList failedToUpdate;
        
        Q_FOREACH( const QString& dev, devices )
        {
            if( updateDevicePermissions( g, dev ) )
                updated.push_back( dev );
            else
                failedToUpdate.push_back( dev );
        }
        
        Q_FOREACH( const QVariant& v, programs )
        {
            ProgramItem program;
            v.value<QDBusArgument>() >> program;
            
            if( updateProgramPermissions( g, program.m_path, program.m_needSuid ) )
                updated.push_back( program.m_path );
            else
                failedToUpdate.push_back( program.m_path );
        }
        
        emit done( updated, failedToUpdate );
    }
    else {
        qDebug() << QString("Not authorized");
        emit authorizationFailed();
    }

    QCoreApplication::instance()->quit();
}

} // namespace Setup
} // namespace K3b

#include "k3bsetupworker.moc"
