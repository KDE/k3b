/***************************************************************************
                          k3bdvdsizetab.h  -  description
                             -------------------
    begin                : Mon Apr 1 2002
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

#ifndef K3BDVDSIZETAB_H
#define K3BDVDSIZETAB_H

#include <qwidget.h>

class K3bDvdCodecData;
class K3bDvdCrop;
class K3bDvdInfoExtend;
class K3bDvdResize;

/**
  *@author Sebastian Trueg
  */

class K3bDvdSizeTab : public QWidget  {
   Q_OBJECT
public: 
    K3bDvdSizeTab( K3bDvdCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDvdSizeTab();
    void show();
private:
    K3bDvdCodecData *m_datas;
    K3bDvdCrop *m_crop;
    K3bDvdInfoExtend *m_info;
    K3bDvdResize *m_resize;

    void setupGui();
};

#endif
