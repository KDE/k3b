/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

#ifndef K3BVCDVIEW_H
#define K3BVCDVIEW_H

#include <qstringlist.h>
#include <qptrlist.h>

// K3b Includes
#include <k3bview.h>

class K3bVcdListView;
class K3bVcdListViewItem;
class QWidget;
class K3bVcdDoc;
class K3bVcdTrack;
class QListViewItem;
class KListView;
class K3bVcdBurnDialog;
class K3bProjectBurnDialog;


class K3bVcdView : public K3bView
{

        Q_OBJECT

    public:
        K3bVcdView( K3bVcdDoc* pDoc, QWidget* parent, const char *name = 0 );
        ~K3bVcdView();

    private:
        K3bVcdDoc* m_doc;

        K3bVcdListView* m_vcdlist;
};

#endif
