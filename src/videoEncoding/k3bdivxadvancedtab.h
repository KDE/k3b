/***************************************************************************
                          k3bdivxadvancedtab.h  -  description
                             -------------------
    begin                : Tue Jul 30 2002
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

#ifndef K3BDIVXADVANCEDTAB_H
#define K3BDIVXADVANCEDTAB_H

#include <qwidget.h>

class K3bDivxCodecData;
class K3bDivxExtSettings;
/**
  *@author Sebastian Trueg
  */

class K3bDivxAdvancedTab : public QWidget  {
   Q_OBJECT
public: 
    K3bDivxAdvancedTab(K3bDivxCodecData *, QWidget *parent=0, const char *name=0);
    ~K3bDivxAdvancedTab();
public slots:
    void slotUpdateView();
private:
    K3bDivxCodecData *m_data;
    K3bDivxExtSettings *m_extension;

    void setupGui();
};

#endif
