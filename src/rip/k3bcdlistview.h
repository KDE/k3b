/***************************************************************************
                          k3bcdlistview.h  -  description
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

#ifndef K3BCDLISTVIEW_H
#define K3BCDLISTVIEW_H

#include <klistview.h>
//class QDragObject;

/**
  *@author Sebastian Trueg
  */

class K3bCDListView : public KListView  {
public: 
    K3bCDListView(QWidget* parent, const char *name);
    ~K3bCDListView();
protected:
    virtual void startDrag();
//    virtual QDragObject *dragObject( );
};

#endif
