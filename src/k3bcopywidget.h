/***************************************************************************
                          k3bcopywidget.h  -  description
                             -------------------
    begin                : Tue Mar 27 2001
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

#ifndef K3BCOPYWIDGET_H
#define K3BCOPYWIDGET_H

#include <qwidget.h>

/**
  *@author Sebastian Trueg
  */

class K3bCopyWidget : public QWidget  {
   Q_OBJECT

public: 
	K3bCopyWidget(QWidget *parent=0, const char *name=0);
	~K3bCopyWidget();
};

#endif
