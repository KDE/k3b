/***************************************************************************
                          k3bcdcopyjob.h  -  description
                             -------------------
    begin                : Sun Mar 17 2002
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

#ifndef K3BCDCOPYJOB_H
#define K3BCDCOPYJOB_H

#include "../k3bjob.h"

/**
  *@author Sebastian Trueg
  */
class K3bCdCopyJob : public K3bBurnJob
{
  Q_OBJECT

 public: 
  K3bCdCopyJob();
  ~K3bCdCopyJob();
};

#endif
