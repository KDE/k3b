/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bbootitem.h"
#include "k3bdatadoc.h"

#include <klocale.h>

#include <qptrlist.h>


K3bBootItem::K3bBootItem( const QString& fileName, K3bDataDoc* doc, K3bDirItem* dir, const QString& k3bName )
  : K3bFileItem( fileName, doc, dir, k3bName ),
    m_noEmulate(false),
    m_noBoot(false),
    m_bootInfoTable(false),
    m_loadSegment(-1),
    m_loadSize(-1),
    m_imageType(FLOPPY)
{
  setExtraInfo( i18n("El Torito Boot image") );
}

K3bBootItem::~K3bBootItem()
{
  doc()->removeBootItem(this);
}
