/***************************************************************************
                          k3bcdlistview.cpp  -  description
                             -------------------
    begin                : Mon Oct 7 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#include "k3bcdlistview.h"

#include <qevent.h>
#include <qdragobject.h>

#include <kdebug.h>


K3bCDListView::K3bCDListView(QWidget* parent, const char *name): KListView( parent, name ) {
}
K3bCDListView::~K3bCDListView(){
}

void K3bCDListView::startDrag(){
    kdDebug() << "EnterDragEvent" << endl;
    QDragObject *d = new QTextDrag( "Test", this );
    d->dragCopy(); // do NOT delete d.
}

QDragObject * K3bCDListView::dragObject(){
    kdDebug() << "(K3bCDListView:dragObject)" << endl;
    return new QTextDrag( "DragObject in ListView", this );
}

