/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef kiotreemodule_h
#define kiotreemodule_h

#include <qobject.h>
#include "kiotree.h"

class QDragObject;
class KioTreeItem;
class KioTreeTopLevelItem;
class KioTree;

/**
 * The base class for KioTree Modules. It defines the interface
 * between the generic KioTree and the particular modules
 * (directory tree, history, bookmarks, ...)
 */
class KioTreeModule
{
public:
    KioTreeModule( KioTree * parentTree )
        : m_pTree( parentTree ) {}
    virtual ~KioTreeModule() {}

    // Handle this new toplevel item [can only be called once currently]
    virtual void addTopLevelItem( KioTreeTopLevelItem * item ) = 0;

    // Open this toplevel item - you don't need to reimplement if
    // you create the item's children right away
    virtual void openTopLevelItem( KioTreeTopLevelItem * ) {}

    // Follow a URL opened in another view - only implement if the module
    // has anything to do with URLs
    virtual void followURL( const KURL & ) {}

    KioTree *tree() const { return m_pTree; }

protected:
    KioTree * m_pTree;
};

#endif
