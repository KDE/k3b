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


K3bFileItem::K3bFileItem( const QString& fileName, K3bDataDoc* doc, K3bDirItem* dir )
	: m_file( fileName )
{
	this->doc = doc;
	m_dir = dir;
	m_next = 0;
	m_prev = 0;
	
	m_isoName = doc->isoName( this );
	m_joiletName = m_rockRidgeName = m_file.name();
}


K3bFileItem::~K3bFileItem()
{
}

bool K3bFileItem::exists() const
{
	return m_file.exists();
}

QString K3bFileItem::absIsoPath()
{
	return m_dir->absIsoPath() + m_isoName;
}
