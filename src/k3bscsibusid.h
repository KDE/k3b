/***************************************************************************
                          k3bscsibusid.h  -  description
                             -------------------
    begin                : Sun Sep 23 2001
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

#ifndef K3BSCSIBUSID_H
#define K3BSCSIBUSID_H

#include <qstring.h>

/**
 * @author Sebastian Trueg
 */
class K3bScsiBusId 
{
 public:
  K3bScsiBusId();
  K3bScsiBusId( int b, int t, int l, const QString& p );
  ~K3bScsiBusId();

  int bus;
  int target;
  int lun;
  QString product;
};

#endif
