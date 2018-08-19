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


#include "k3bprojectinterface.h"
#include "k3bprojectinterfaceadaptor.h"
#include "k3bburnprogressdialog.h"
#include "k3bdoc.h"
#include "k3bview.h"
#include "k3bmsf.h"
#include "k3bcore.h"
#include "k3bdevicemanager.h"
#include "k3bjob.h"

#include <QTimer>
#include <QDBusConnection>

namespace K3b {

ProjectInterface::ProjectInterface( Doc* doc, const QString& dbusPath )
:
    QObject( doc ),
    m_doc( doc )
{
    static int id = 0;
    if( !dbusPath.isEmpty() ) {
        m_dbusPath = dbusPath;
    }
    else {
        m_dbusPath = QString( "/projects/%1" ).arg( id );
        ++id;
    }

    new K3bProjectInterfaceAdaptor( this );
    QDBusConnection::sessionBus().registerObject( m_dbusPath, this );
}


ProjectInterface::~ProjectInterface()
{
    QDBusConnection::sessionBus().unregisterObject( m_dbusPath );
}


QString ProjectInterface::dbusPath() const
{
    return m_dbusPath;
}


void ProjectInterface::addUrls( const QStringList& urls )
{
    QList<QUrl> urlList;
    for( auto& url : urls ) { urlList.push_back( QUrl::fromUserInput( url ) ); }
    m_doc->addUrls( urlList );
}


void ProjectInterface::addUrl( const QString& url )
{
    m_doc->addUrl( QUrl::fromLocalFile(url) );
}


void ProjectInterface::burn()
{
    // we want to return this method immediately
    QTimer::singleShot( 0, m_doc->view(), SLOT(slotBurn()) );
}


bool ProjectInterface::directBurn()
{
    if( m_doc->burner() ) {
        JobProgressDialog* dlg = 0;
        if( m_doc->onlyCreateImages() )
            dlg = new JobProgressDialog( m_doc->view() );
        else
            dlg = new BurnProgressDialog( m_doc->view() );

        Job* job = m_doc->newBurnJob( dlg );

        dlg->startJob( job );

        delete job;
        delete dlg;

        return true;
    }
    else
        return false;
}


void ProjectInterface::setBurnDevice( const QString& name )
{
    if( Device::Device* dev = k3bcore->deviceManager()->findDevice( name ) )
        m_doc->setBurner( dev );
}


int ProjectInterface::length() const
{
    return m_doc->length().lba();
}


KIO::filesize_t ProjectInterface::size() const
{
    return m_doc->size();
}


const QString& ProjectInterface::imagePath() const
{
    return m_doc->tempDir();
}


QString ProjectInterface::projectType() const
{
    switch( m_doc->type() ) {
        case Doc::AudioProject:
            return "audiocd";
        case Doc::DataProject:
            return "data";
        case Doc::MixedProject:
            return "mixedcd";
        case Doc::VcdProject:
            return "videocd";
        case Doc::MovixProject:
            return "emovix";
        case Doc::VideoDvdProject:
            return "videodvd";
        default:
            return "unknown";
    }
}

} // namespace K3b


