/***************************************************************************
                          k3bfileitem.cpp  -  description
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

#include "k3bfileitem.h"
#include "k3bdatadoc.h"
#include "k3bdiritem.h"

#include <qfileinfo.h>
#include <qstring.h>

#include <kurl.h>


K3bFileItem::K3bFileItem( const QString& filePath, K3bDataDoc* doc, K3bDirItem* dir, const QString& k3bName )
  : KFileItem( -1, -1, filePath ), K3bDataItem( doc, dir )
{
  if( k3bName.isEmpty() )
    m_k3bName = QFileInfo(filePath).fileName();
  else
    m_k3bName = k3bName;

//	m_isoName = doc()->isoName( this );
//	m_joiletName = m_rockRidgeName = m_file.name();
}


K3bFileItem::~K3bFileItem()
{
}


long K3bFileItem::k3bSize() const
{
	return size();
}


bool K3bFileItem::exists() const
{
	return isLocalFile();
}

QString K3bFileItem::absIsoPath()
{
//	return m_dir->absIsoPath() + m_isoName;
	return QString::null;
}


//K3bDataItem* K3bFileItem::nextSibling()
//{
//	K3bDataItem* _item = this;
//	K3bDataItem* _parentItem = parent();
//	
//	while( _parentItem ) {
//		if( K3bDataItem* i = _parentItem->nextChild( _item ) )
//			return i;
//		
//		_item = _parentItem;
//		_parentItem = _item->parent();
//	}
//
//	return 0;
//		
//	if( parent() ) {
//		if( K3bDataItem* i = parent()->nextChild( this ) )
//			return i;
//		else {
//			// test if parent() has a parent
//			if( parent()->parent() )
//				return parent()->parent()->nextChild( parent() );
//			else
//				return 0;
//		}
//	}
//	else {
//		qDebug( "(K3bFileItem) ERROR: K3bFileItem without parent dirItem!!");
//		return 0;
//	}
//}


QString K3bFileItem::localPath()
{
	return url().path();
}

K3bDirItem* K3bFileItem::addDataItem( K3bDataItem* item )
{
	return parent()->addDataItem( item );
}
