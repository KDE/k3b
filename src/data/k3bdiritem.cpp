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


K3bDirItem::K3bDirItem(const QString& name, K3bDirItem* parentDir)
	: K3bDataItem( parentDir ), m_name( name )
{
}

K3bDirItem::~K3bDirItem()
{
}
