/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3burlnavigator.h"
#include "k3bcore.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bglobals.h"
#include "k3bmediacache.h"
#include "k3bmedium.h"

#include <KMountPoint>
#include <QDir>

K3b::UrlNavigator::UrlNavigator( KFilePlacesModel* model, QWidget* parent )
    : KUrlNavigator( model, QUrl::fromLocalFile(QDir::home().absolutePath()), parent )
{
    // Currently we don't support burning from custom protocols so let's filter them out
    KUrlNavigator::setSupportedSchemes( QStringList() << "file" << "audiocd" );
    
	connect( this, SIGNAL(urlChanged(QUrl)), this, SLOT(urlActivated(QUrl)) );
}

K3b::UrlNavigator::~UrlNavigator()
{
}

void K3b::UrlNavigator::setDevice( K3b::Device::Device* dev )
{
    // Check if device is mounted. If so, switch to the mount path
    if( KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( dev->blockDeviceName() ) )
    {
        QString mntPath = mountPoint->mountPoint();
        if( !mntPath.isEmpty() ) {
            setLocationUrl( QUrl::fromLocalFile( mntPath ) );
            return;
        }
    }
    
    const Medium& medium = k3bcore->mediaCache()->medium( dev );
    if( medium.content() & Medium::ContentAudio )
    {
        setLocationUrl( QUrl( "audiocd:/" ) );
    }
}

void K3b::UrlNavigator::urlActivated( const QUrl& url )
{
    if( url.scheme() == "audiocd" )
    {
        Q_FOREACH( Device::Device* device, k3bcore->deviceManager()->cdReader() )
        {
            const Medium& medium = k3bcore->mediaCache()->medium( device );
            if( medium.content() & Medium::ContentAudio )
            {
                emit activated( device );
                return;
            }
        }
    }
    
    emit activated( url );
}

#include "moc_k3burlnavigator.cpp"
