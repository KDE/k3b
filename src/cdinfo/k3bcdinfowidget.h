/***************************************************************************
                          k3bcdinfowidget.h  -  description
                             -------------------
    begin                : Sun Apr 22 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3BCDINFOWIDGET_H
#define K3BCDINFOWIDGET_H

#include <qwidget.h>

/**
  *@author Sebastian Trueg
  */

class K3bCDInfoWidget : public QWidget  {
   Q_OBJECT
public: 
	K3bCDInfoWidget(QWidget *parent=0, const char *name=0);
	~K3bCDInfoWidget();
};

#endif
