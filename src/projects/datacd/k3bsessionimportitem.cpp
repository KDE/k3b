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

#include "k3bsessionimportitem.h"
#include "k3bfileitem.h"
#include "k3bdiritem.h"

#include <k3biso9660.h>


K3bSessionImportItem::K3bSessionImportItem( const K3bIso9660File* isoF, K3bDataDoc* doc, K3bDirItem* dir )
  : K3bDataItem( doc, dir ),
    m_replaceItem(0),
    m_size( isoF->size() )

{
  setK3bName( isoF->name() );

  // add automagically like a qlistviewitem
  if( parent() )
    parent()->addDataItem( this );
}


K3bSessionImportItem::~K3bSessionImportItem()
{
  if( m_replaceItem )
    m_replaceItem->setReplacedItemFromOldSession(0);

  // remove this from parentdir
  if( parent() )
    parent()->takeDataItem( this );
}
