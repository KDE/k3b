/* 
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BMODELTYPES_H
#define K3BMODELTYPES_H

#include <Qt>

namespace K3b
{
    enum ItemDataRoles
    {
        ItemTypeRole = Qt::UserRole,
        CustomFlagsRole = Qt::UserRole + 1
    };

    enum ItemType
    {
        DirItem,
        FileItem
    };

    enum ItemFlags
    {
        ItemIsRemovable = 1
    };

}
#endif
