/***************************************************************************
                          k3bcddbemptydbtab.h  -  description
                             -------------------
    begin                : Tue Feb 19 2002
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

#ifndef K3BCDDBEMPTYDBTAB_H
#define K3BCDDBEMPTYDBTAB_H

#include <qwidget.h>

/**
  *@author Sebastian Trueg
  */

class K3bCddbEmptyDbTab : public QWidget  {
   Q_OBJECT
public: 
	K3bCddbEmptyDbTab(QWidget *parent=0, const char *name=0);
	~K3bCddbEmptyDbTab();
private:
    void setup();
};

#endif
