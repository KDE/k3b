/***************************************************************************
                          k3bdvdinfoextend.h  -  description
                             -------------------
    begin                : Thu Apr 4 2002
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

#ifndef K3BDVDINFOEXTEND_H
#define K3BDVDINFOEXTEND_H

#include <qwidget.h>
#include <k3bdvdinfo.h>

class QLabel;

/**
  *@author Sebastian Trueg
  */

class K3bDvdInfoExtend : public K3bDvdInfo  {
   Q_OBJECT
public: 
    K3bDvdInfoExtend(QWidget *parent=0, const char *name=0);
    ~K3bDvdInfoExtend();
private:
    QLabel *m_quality;

    void setupGui();

};

#endif
