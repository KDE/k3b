/***************************************************************************
                          k3bpatternoptiontab.h  -  description
                             -------------------
    begin                : Sat May 4 2002
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

#ifndef K3BPATTERNOPTIONTAB_H
#define K3BPATTERNOPTIONTAB_H

#include "base_k3bpatternoptiontab.h"
#include "../k3bcddb.h"


/**
  *@author Sebastian Trueg
  */
class K3bPatternOptionTab : public base_K3bPatternOptionTab
{
  Q_OBJECT

 public: 
  K3bPatternOptionTab( QWidget *parent = 0, const char *name = 0 );
  ~K3bPatternOptionTab();

  void readSettings();
  void apply();

 protected slots:
  void slotUpdateExample();

 private:
  K3bCddbEntry m_exampleEntry;
};

#endif
