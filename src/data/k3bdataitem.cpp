/***************************************************************************
                          k3bdataitem.cpp  -  description
                             -------------------
    begin                : Wed Apr 25 2001
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

#include "k3bdataitem.h"
#include "k3bdiritem.h"


K3bDataItem::K3bDataItem( K3bDataDoc* doc, K3bDirItem* parent )
{
	m_parentDir = parent;
	m_doc = doc;
	
	// add automagically like a qlistviewitem
	if( parent )
		parent->addDataItem( this );
}

K3bDataItem::~K3bDataItem()
{
	// remove his from parentdir
	if( m_parentDir )
		m_parentDir->takeDataItem( this );
}


void K3bDataItem::setK3bName( const QString& name ){
	m_k3bName = name;
}


const QString& K3bDataItem::k3bName(){
	return m_k3bName;
}


QString K3bDataItem::k3bPath()
{
	if( !m_parentDir )
		return k3bName();
	else
		return m_parentDir->k3bPath() + k3bName();
}


K3bDataItem* K3bDataItem::nextSibling()
{
	K3bDataItem* _item = this;
	K3bDirItem* _parentItem = parent();
	
	while( _parentItem ) {
		if( K3bDataItem* i = _parentItem->nextChild( _item ) )
			return i;
		
		_item = _parentItem;
		_parentItem = _item->parent();
	}

	return 0;
}
