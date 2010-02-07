/*
 *
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 * Copyright (C) 2010 Dario Freddi <drf@kde.org>
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

#include "k3bsetupworker.h"
#include "k3bsetupprogramitem.h"

#include <QFile>
#include <QString>

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
  
ActionReply Worker::save(const QVariantMap &args)
{
    QString burningGroup = args["burningGroup"].toString();
    QStringList devices = args["devices"].toStringList();
    QVariantList programs = args["programs"].value<QVariantList>();
        
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
    
    ActionReply reply = ActionReply::SuccessReply;
    QVariantMap data;
    data["updated"] = updated;
    data["failedToUpdate"] = failedToUpdate;
    reply.setData(data);
    
    return reply;
}

} // namespace Setup
} // namespace K3b

KDE4_AUTH_HELPER_MAIN("org.kde.kcontrol.k3bsetup", K3b::Setup::Worker)

#include "k3bsetupworker.moc"
