/***************************************************************************
                          k3bdataitem.h  -  description
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

#ifndef K3BDATAITEM_H
#define K3BDATAITEM_H


class K3bDirItem;
class K3bDataDoc;

/**
  *@author Sebastian Trueg
  */

class K3bDataItem
{
public: 
	K3bDataItem( K3bDataDoc* doc, K3bDirItem* parent = 0 );
	virtual ~K3bDataItem();
	
	K3bDirItem* parent() { return m_parentDir; }
//	void setParentDir( K3bDirItem* dir ) { m_parentDir = dir; }
	
	K3bDataDoc* doc() const { return m_doc; }
	
private:
	K3bDirItem* m_parentDir;
	K3bDataDoc* m_doc;
};

#endif
