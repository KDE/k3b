/*
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdebuggingoutputcache.h"

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
#include <k3bcore.h>
#include <k3bversion.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kglobalsettings.h>
#include <kdeversion.h>


static const int s_maxCache = 10*1024*1024; // 10 MB max cache size

class K3bDebuggingOutputCache::Private
{
public:
    Private()
        : stderrEnabled( false ),
          cacheSize( 0 ) {
    }

    bool stderrEnabled;
    QMap<QString, QString> groups;
    QMap<QString, QString> lastMessages;
    QMap<QString, int> lastMessageCount;
    int cacheSize;
};


K3bDebuggingOutputCache::K3bDebuggingOutputCache()
    : d( new Private() )
{
}


K3bDebuggingOutputCache::~K3bDebuggingOutputCache()
{
    delete d;
}


void K3bDebuggingOutputCache::clear()
{
    d->groups.clear();
    d->lastMessages.clear();
    d->lastMessageCount.clear();
    d->cacheSize = 0;

    addOutput( "System", "K3b Version: " + k3bcore->version() );
    addOutput( "System", "KDE Version: " + QString(KDE::versionString()) );
    addOutput( "System", "QT Version:  " + QString(qVersion()) );
    addOutput( "System", "Kernel:      " + K3b::kernelVersion() );

    // devices in the logfile
    for( Q3PtrListIterator<K3bDevice::Device> it( k3bcore->deviceManager()->allDevices() ); *it; ++it ) {
        K3bDevice::Device* dev = *it;
        addOutput( "Devices",
                   QString( "%1 (%2, %3) [%5] [%6] [%7]" )
                   .arg( dev->vendor() + " " + dev->description() + " " + dev->version() )
                   .arg( dev->blockDeviceName() )
                   .arg( dev->genericDevice() )
                   .arg( K3bDevice::deviceTypeString( dev->type() ) )
                   .arg( K3bDevice::mediaTypeString( dev->supportedProfiles() ) )
                   .arg( K3bDevice::writingModeString( dev->writingModes() ) ) );
    }
}


void K3bDebuggingOutputCache::addOutput( const QString& group, const QString& line )
{
    if ( d->lastMessages[group] == line ) {
        d->lastMessageCount[group]++;
    }
    else {
        if ( d->lastMessageCount.contains( group ) &&
             d->lastMessageCount[group] > 1 ) {
            d->groups[group].append( QString( "=== last message repeated %1 times. ===\n" ).arg( d->lastMessageCount[group]   ) );
        }
        d->lastMessageCount[group] = 1;
        d->lastMessages[group] = line;

        d->cacheSize += line.length();
        if ( d->cacheSize > s_maxCache &&
            d->cacheSize - ( int )line.length() <= s_maxCache ) {
            d->groups[defaultGroup()].append( "=== K3b debugging output cache overflow ===\n" );
        }
        else if ( d->cacheSize <= s_maxCache ) {
            d->groups[group].append( line + '\n' );
        }
    }
}


K3bDebuggingOutputCache& K3bDebuggingOutputCache::operator<<( const QString& line )
{
    addOutput( defaultGroup(), line );
    return *this;
}


QString K3bDebuggingOutputCache::toString() const
{
    QString s;
    for ( QMap<QString, QString>::const_iterator it = d->groups.constBegin();
          it != d->groups.constEnd(); ++it ) {
        if ( !s.isEmpty() )
            s.append( '\n' );
        s.append( it.key() + '\n' );
        s.append( "-----------------------\n" );
        s.append( *it );
    }
    return s;
}


QMap<QString, QString> K3bDebuggingOutputCache::toGroups() const
{
    return d->groups;
}


bool K3bDebuggingOutputCache::stderrEnabled() const
{
    return d->stderrEnabled;
}


void K3bDebuggingOutputCache::enableStderr( bool b )
{
    d->stderrEnabled = b;
}


QString K3bDebuggingOutputCache::defaultGroup()
{
    return "Misc";
}
