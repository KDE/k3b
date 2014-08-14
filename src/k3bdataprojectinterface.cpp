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

#include "k3bdataprojectinterface.h"
#include "k3bdataprojectinterfaceadaptor.h"
#include "k3bdatadoc.h"
#include "k3bdiritem.h"
#include "k3bisooptions.h"
#include <QList>

namespace K3b {


DataProjectInterface::DataProjectInterface( DataDoc* doc, const QString& dbusPath )
:
    ProjectInterface( doc, dbusPath ),
    m_dataDoc(doc)
{
    new K3bDataProjectInterfaceAdaptor( this );
}


bool DataProjectInterface::createFolder( const QString& name )
{
    return createFolder( name, "/" );
}


bool DataProjectInterface::createFolder( const QString& name, const QString& dir )
{
    DataItem* p = m_dataDoc->root()->findByPath( dir );
    if( p && p->isDir() && !static_cast<DirItem*>(p)->find( name ) ) {
        m_dataDoc->addEmptyDir( name, static_cast<DirItem*>(p) );
        return true;
    }
    return false;
}


void DataProjectInterface::addUrl( const QString& url, const QString& dir )
{
    addUrls( QStringList(url), dir );
}


void DataProjectInterface::addUrls( const QStringList& urls, const QString& dir )
{
    DataItem* p = m_dataDoc->root()->findByPath( dir );
    QList<QUrl> urlList;
    for( auto& url : urls ) { urlList.push_back( QUrl::fromUserInput( url ) ); }
    if( p && p->isDir() )
        m_dataDoc->addUrlsToDir( urlList, static_cast<DirItem*>(p) );
}


bool DataProjectInterface::removeItem( const QString& path )
{
    DataItem* p = m_dataDoc->root()->findByPath( path );
    if( p && p->isRemoveable() ) {
        m_dataDoc->removeItem( p );
        return true;
    }
    else
        return false;
}


bool DataProjectInterface::renameItem( const QString& path, const QString& newName )
{
    DataItem* p = m_dataDoc->root()->findByPath( path );
    if( p && p->isRenameable() && !newName.isEmpty() ) {
        p->setK3bName( newName );
        return true;
    }
    else
        return false;
}


void DataProjectInterface::setVolumeID( const QString& id )
{
    m_dataDoc->setVolumeID( id );
}

bool DataProjectInterface::isFolder( const QString& path ) const
{
    DataItem* p =  m_dataDoc->root()->findByPath( path );
    if( p )
        return p->isDir();
    else
        return false;
}


QStringList DataProjectInterface::children( const QString& path ) const
{
    QStringList l;
    DataItem* item =  m_dataDoc->root()->findByPath( path );
    if( item && item->isDir() ) {
        QList<DataItem*> const& cl = static_cast<DirItem*>(item)->children();
        Q_FOREACH( DataItem* item, cl ) {
            l.append( item->k3bName() );
        }
    }

    return l;
}


bool DataProjectInterface::setSortWeight( const QString& path, long weight ) const
{
    K3b::DataItem* item =  m_dataDoc->root()->findByPath( path );
    if( item ) {
        item->setSortWeight( weight );
        return true;
    }
    else
        return false;
}

} // namespace K3b


