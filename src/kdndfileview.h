/***************************************************************************
                          kdndfiledetailview.h  -  description
                             -------------------
    begin                : Sat Apr 20 2002
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

#ifndef KDNDFILEVIEW_H
#define KDNDFILEVIEW_H

#include <kfiledetailview.h>
#include <kfileiconview.h>


/**
 * adds Drag'n'Drop support
  * @author Sebastian Trueg
  */
class KDndFileDetailView : public KFileDetailView
{
  Q_OBJECT

 public: 
  KDndFileDetailView( QWidget* parent, const char* name );
  ~KDndFileDetailView();
  
 protected:
  QDragObject* dragObject();
};


class KDndFileIconView : public KFileIconView
{
  Q_OBJECT

 public:
  KDndFileIconView( QWidget* parent = 0, const char* name = 0);
  ~KDndFileIconView();

 protected:
  QDragObject* dragObject();
};

#endif
