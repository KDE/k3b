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

//#include <kurl.h>


K3bFileItem::K3bFileItem( const QString& filePath, K3bDataDoc* doc, K3bDirItem* dir )
	: KFileItem( -1, -1, filePath ), K3bDataItem( doc, dir )
{
	setK3bName( name() );
//	m_isoName = doc()->isoName( this );
//	m_joiletName = m_rockRidgeName = m_file.name();
}


K3bFileItem::~K3bFileItem()
{
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
