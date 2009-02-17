/*
 *
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

#include "k3burlnavigator.h"
#include "k3bcore.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bglobals.h"
#include "k3bplacesmodel.h"

#include <kmountpoint.h>
#include <QDir>

K3bUrlNavigator::K3bUrlNavigator( KFilePlacesModel* model, QWidget* parent )
    : KUrlNavigator( model, KUrl(QDir::home().absolutePath()), parent )
{
	connect( this, SIGNAL(urlChanged(const KUrl&)), this, SLOT(urlActivated(const KUrl&)) );
}

K3bUrlNavigator::~K3bUrlNavigator()
{
}

void K3bUrlNavigator::setDevice( K3bDevice::Device* dev )
{
	// Check if device is mounted. If so, switch to the mount path
	KSharedPtr<KMountPoint> mountPoint = KMountPoint::currentMountPoints().findByDevice( dev->blockDeviceName() );
	if( !mountPoint.isNull() )
	{
		QString mntPath = mountPoint->mountPoint();
		if( !mntPath.isEmpty() ) {
			setUrl( KUrl( mntPath ) );
		}
	}
}

void K3bUrlNavigator::urlActivated( const KUrl& url )
{
    emit activated( url );
}

#include "k3burlnavigator.moc"
