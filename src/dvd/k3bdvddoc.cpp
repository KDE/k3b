/***************************************************************************
                          k3bdvddoc.cpp  -  description
                             -------------------
    begin                : Sun Mar 31 2002
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

#include "k3bdvddoc.h"
#include "../k3bview.h"
#include "k3bdvdview.h"

K3bDvdDoc::K3bDvdDoc( QObject *parent) : K3bDoc( parent ){
}

K3bDvdDoc::~K3bDvdDoc(){
}

bool K3bDvdDoc::newDocument(){
    return true;
}

K3bView* K3bDvdDoc::newView( QWidget* parent ){
    return new K3bDvdView( this, parent );
}

void K3bDvdDoc::addView(K3bView* view){
    K3bDoc::addView( view );
}

#include "k3bdvddoc.moc"
