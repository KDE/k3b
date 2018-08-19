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

#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bcore.h"
#include "k3bversion.h"
#include "k3bglobals.h"

#include <KLocalizedString>
#include <kcoreaddons_version.h>


static const int s_maxCache = 10*1024*1024; // 10 MB max cache size

class K3b::DebuggingOutputCache::Private
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


K3b::DebuggingOutputCache::DebuggingOutputCache()
    : d( new Private() )
{
}


K3b::DebuggingOutputCache::~DebuggingOutputCache()
{
    delete d;
}


void K3b::DebuggingOutputCache::clear()
{
    d->groups.clear();
    d->lastMessages.clear();
    d->lastMessageCount.clear();
    d->cacheSize = 0;

    if (k3bcore == Q_NULLPTR)
       return; 
    addOutput( QLatin1String( "System" ), QString::fromLatin1( "K3b Version: %1" ).arg(k3bcore->version()) );
    addOutput( QLatin1String( "System" ), QString::fromLatin1( "KDE Version: %1" ).arg(KCOREADDONS_VERSION_STRING) );
    addOutput( QLatin1String( "System" ), QString::fromLatin1( "Qt Version:  %1" ).arg(qVersion()) );
    addOutput( QLatin1String( "System" ), QString::fromLatin1( "Kernel:      %1" ).arg(K3b::kernelVersion()) );

    // devices in the logfile
    QList<K3b::Device::Device *> items(k3bcore->deviceManager()->allDevices());
    for( QList<K3b::Device::Device *>::const_iterator it = items.constBegin();
       it != items.constEnd(); ++it ) {

        K3b::Device::Device* dev = *it;
        addOutput( "Devices",
                   QString( "%1 (%2, %3) [%5] [%6] [%7]" )
                   .arg( dev->vendor() + ' ' + dev->description() + ' ' + dev->version() )
                   .arg( dev->blockDeviceName() )
                   .arg( K3b::Device::deviceTypeString( dev->type() ) )
                   .arg( K3b::Device::mediaTypeString( dev->supportedProfiles() ) )
                   .arg( K3b::Device::writingModeString( dev->writingModes() ) ) );
    }
}


void K3b::DebuggingOutputCache::addOutput( const QString& group, const QString& line )
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


K3b::DebuggingOutputCache& K3b::DebuggingOutputCache::operator<<( const QString& line )
{
    addOutput( defaultGroup(), line );
    return *this;
}


QString K3b::DebuggingOutputCache::toString() const
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


QMap<QString, QString> K3b::DebuggingOutputCache::toGroups() const
{
    return d->groups;
}


bool K3b::DebuggingOutputCache::stderrEnabled() const
{
    return d->stderrEnabled;
}


void K3b::DebuggingOutputCache::enableStderr( bool b )
{
    d->stderrEnabled = b;
}


QString K3b::DebuggingOutputCache::defaultGroup()
{
    return "Misc";
}
