/***************************************************************************
                          k3bdiritem.h  -  description
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

#ifndef K3BDIRITEM_H
#define K3BDIRITEM_H


#include <qstring.h>
#include <qlist.h>

#include "k3bdataitem.h"

class K3bDataDoc;

/**
  *@author Sebastian Trueg
  */

class K3bDirItem : public K3bDataItem
{
public: 
	K3bDirItem( const QString& name, K3bDataDoc*, K3bDirItem* parentDir = 0 );
	~K3bDirItem();
	
	QList<K3bDataItem>* children() const { return m_children; }
	const QString& name() { return m_name; }
	void addDataItem( K3bDataItem* item );
	K3bDataItem* takeDataItem( K3bDataItem* item );
	K3bDataItem* takeDataItem( int index );
	
private:
	QString m_name;
	QList<K3bDataItem>* m_children;
};

#endif
