/***************************************************************************
                          k3bstickybutton.h  -  description
                             -------------------
    begin                : Sun Apr 1 2001
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

#ifndef K3BSTICKYBUTTON_H
#define K3BSTICKYBUTTON_H

#include <qtoolbutton.h>

/**
  *@author Sebastian Trueg
  */

class K3bStickyButton : public QToolButton  {

   Q_OBJECT

public:
	K3bStickyButton(QWidget *parent=0, const char *name=0);
	~K3bStickyButton();
	
public slots:
  void setOn( bool );
};

#endif
