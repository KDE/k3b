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
#include "k3bcdview.h"

#include <qevent.h>
#include <qdragobject.h>

#include <kdebug.h>



K3bCDListView::K3bCDListView(QWidget* parent, const char *name): KListView( parent, name ) {
}
K3bCDListView::~K3bCDListView(){
}

void K3bCDListView::startDrag(){
    
    /*
    QPtrList<QListViewItem> selectedList = selectedItems();
    if( selectedList.isEmpty() ){
        kdDebug() << "(K3bCDListView) no item selected. " << endl;
        //KMessageBox::critical( this, i18n("Ripping Error"), i18n("Please select the title to rip."), i18n("OK") );
        return;
    }

    //QValueList<int> trackNumbers;
    for( QPtrListIterator<QListViewItem> it( selectedList ); it.current(); ++it ) {
        buf.append( it.current()->text(0) + "," );
        //trackNumbers.append( it.current()->text(0).toInt() );
        kdDebug() << "(K3bCDListView) Tracknumber to rip: "<<  it.current()->text(0) << endl;
    }
    buf = buf.left( buf.length() -1 );
    */
    QString buf( CD_DRAG );
    QDragObject *d = new QTextDrag( buf, this );
    d->dragCopy(); // do NOT delete d.
}

/*
QDragObject * K3bCDListView::dragObject(){
    kdDebug() << "(K3bCDListView:dragObject)" << endl;
    return new QTextDrag( "DragObject in ListView", this );
}
*/
