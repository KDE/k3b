/***************************************************************************
                          K3bDivxOptionTab.h  -  description
                             -------------------
    begin                : Mon Feb 17 2003
    copyright            : (C) 2003 by Sebastian Trueg
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

#ifndef _K3BDIVXOPTIONTAB_H_
#define _K3BDIVXOPTIONTAB_H_

#include <base_k3bdivxoptiontab.h>

/**
 * 
 * Sebastian Trueg
 **/
class K3bDivxOptionTab : public base_K3bDivxOptions
{
Q_OBJECT
public:
  K3bDivxOptionTab(QWidget* parent,  const char* name );
  ~K3bDivxOptionTab();
public slots:
  void readSettings();
  void saveSettings();
};

#endif
