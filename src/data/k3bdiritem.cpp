/***************************************************************************
                          k3bdiritem.cpp  -  description
                             -------------------
    begin                : Sat Apr 21 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bdiritem.h"

#include <qstring.h>
#include <qlist.h>

K3bDirItem::K3bDirItem(const QString& name, K3bDataDoc* doc, K3bDirItem* parentDir)
	: K3bDataItem( doc, parentDir )
{
	m_children = new QList<K3bDataItem>();
	setK3bName( name );
}

K3bDirItem::~K3bDirItem()
{
	// delete all children
	m_children->setAutoDelete( true );
	delete m_children;
}

void K3bDirItem::addDataItem( K3bDataItem* item )
{
	m_children->append( item );
}

K3bDataItem* K3bDirItem::takeDataItem( K3bDataItem* item )
{
	int x = m_children->find( item );
	if( x > -1 )
		return m_children->take( x );
	else
		return 0;
}

K3bDataItem* K3bDirItem::takeDataItem( int index )
{
	return m_children->take( index );
}
