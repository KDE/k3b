/* 
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_SESSION_IMPORT_ITEM_H_
#define _K3B_SESSION_IMPORT_ITEM_H_


#include "k3bspecialdataitem.h"


class K3bDataDoc;
class K3bFileItem;
class K3bDirItem;
class K3bIso9660File;


class K3bSessionImportItem : public K3bSpecialDataItem
{
public:
    K3bSessionImportItem( const K3bIso9660File*, K3bDataDoc* doc, K3bDirItem* );
    K3bSessionImportItem( const K3bSessionImportItem& );
    ~K3bSessionImportItem();

    K3bDataItem* copy() const;

    K3bFileItem* replaceItem() const { return m_replaceItem; }
    void setReplaceItem( K3bFileItem* item ) { m_replaceItem = item; }

    bool isFile() const { return false; }
    bool isFromOldSession() const { return true; }

    bool isRemoveable() const { return false; }
    bool isMoveable() const { return false; }
    bool isRenameable() const { return false; }
    bool isHideable() const { return false; }
    bool writeToCd() const { return false; }

private:
    K3bFileItem* m_replaceItem;
};

#endif
