/***************************************************************************
                          k3bdatadoc.h  -  description
                             -------------------
    begin                : Sun Apr 22 2001
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

#ifndef K3BDATADOC_H
#define K3BDATADOC_H

#include "../k3bdoc.h"

/**
  *@author Sebastian Trueg
  */

class K3bDataDoc : public K3bDoc
{
public:
	K3bDataDoc( QObject* parent );
	~K3bDataDoc();
};

#endif
