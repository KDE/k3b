/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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


#include "k3binterface.h"
#include "k3binterfaceadaptor.h"
#include "k3b.h"
#include "k3bapplication.h"
#include "k3bcore.h"
#include "k3bdevicemanager.h"
#include "k3bdoc.h"
#include "k3bglobals.h"
#include "k3bprojectmanager.h"
#include "k3bview.h"

#include <QList>
#include <QTimer>
#include <QDBusConnection>

namespace K3b {


Interface::Interface(  MainWindow* main  )
:
    QObject( main ),
    m_main( main )
{
    new K3bInterfaceAdaptor( this );
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( "/MainWindow", this );
    dbus.registerService( "org.k3b.k3b" );
}


Interface::~Interface()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterObject( "/MainWindow" );
    dbus.unregisterService( "org.k3b.k3b" );
}


QString Interface::createDataProject()
{
    ProjectManager* projectManager = k3bappcore->projectManager();
    return projectManager->dbusPath( projectManager->createProject( Doc::DataProject ) );
}


QString Interface::createAudioProject()
{
    ProjectManager* projectManager = k3bappcore->projectManager();
    return projectManager->dbusPath( projectManager->createProject( Doc::AudioProject ) );
}


QString Interface::createMixedProject()
{
    ProjectManager* projectManager = k3bappcore->projectManager();
    return projectManager->dbusPath( projectManager->createProject( Doc::MixedProject ) );
}


QString Interface::createVcdProject()
{
    ProjectManager* projectManager = k3bappcore->projectManager();
    return projectManager->dbusPath( projectManager->createProject( Doc::VcdProject ) );
}


QString Interface::createMovixProject()
{
    ProjectManager* projectManager = k3bappcore->projectManager();
    return projectManager->dbusPath( projectManager->createProject( Doc::MovixProject ) );
}


QString Interface::createVideoDvdProject()
{
    ProjectManager* projectManager = k3bappcore->projectManager();
    return projectManager->dbusPath( projectManager->createProject( Doc::VideoDvdProject ) );
}


QString Interface::currentProject()
{
    View* view = m_main->activeView();
    if( view )
        return k3bappcore->projectManager()->dbusPath( view->doc() );
    else
        return QString();
}


QString Interface::openProject( const QString& url )
{
    Doc* doc = k3bappcore->projectManager()->openProject( QUrl( url ) );
    if( doc )
        return k3bappcore->projectManager()->dbusPath( doc );
    else
        return QString();
}


QStringList Interface::projects()
{
    QStringList paths;
    QList<Doc*> docs = k3bappcore->projectManager()->projects();
    Q_FOREACH( Doc* doc, docs ) {
        paths.push_back( k3bappcore->projectManager()->dbusPath( doc ) );
    }
    return paths;
}


void Interface::copyMedium()
{
    m_main->slotMediaCopy();
}


void Interface::copyMedium( const QString& dev )
{
    m_main->mediaCopy( k3bcore->deviceManager()->findDeviceByUdi( dev ) );
}


void Interface::formatMedium()
{
    m_main->slotFormatMedium();
}


void Interface::writeImage()
{
    m_main->slotWriteImage();
}


void Interface::writeImage( const QString& url )
{
    m_main->slotWriteImage( QUrl( url ) );
}


void Interface::audioCdRip()
{
    m_main->slotCddaRip();
}


void Interface::audioCdRip( const QString& dev )
{
    m_main->cddaRip( k3bcore->deviceManager()->findDeviceByUdi( dev ) );
}


void Interface::videoCdRip()
{
    m_main->slotVideoCdRip();
}


void Interface::videoCdRip( const QString& dev )
{
    m_main->videoCdRip( k3bcore->deviceManager()->findDeviceByUdi( dev ) );
}


void Interface::videoDvdRip()
{
    m_main->slotVideoDvdRip();
}


void Interface::videoDvdRip( const QString& dev )
{
    m_main->videoDvdRip( k3bcore->deviceManager()->findDeviceByUdi( dev ) );
}


void Interface::addUrls( const QStringList& urls )
{
    QList<QUrl> urlList;
    for( auto& url : urls ) { urlList.push_back( QUrl::fromUserInput( url ) ); }
    m_main->addUrls( urlList );
}


void Interface::addUrl( const QString& url )
{
    QStringList urls;
    urls.push_back( url );
    addUrls( urls );
}


bool Interface::blocked() const
{
    return k3bcore->jobsRunning();
}

} // namespace K3b


