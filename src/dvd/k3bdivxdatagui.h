/***************************************************************************
                          k3bdivxdatagui.h  -  description
                             -------------------
    begin                : Sun Apr 21 2002
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

#ifndef K3BDIVXDATAGUI_H
#define K3BDIVXDATAGUI_H

#include <qwidget.h>
#include <qgroupbox.h>

class K3bDvdCodecData;

/**
  *@author Sebastian Trueg
  */

class K3bDivXDataGui : public QGroupBox  {
public:
    K3bDivXDataGui(QWidget *parent=0, const char *name=0);
    ~K3bDivXDataGui();
    virtual void updateData( K3bDvdCodecData *data ) = 0;
};

#endif
