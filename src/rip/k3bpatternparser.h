/***************************************************************************
                          k3bpatternparser.h  -  description
                             -------------------
    begin                : Sun Dec 2 2001
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

#ifndef K3BPATTERNPARSER_H
#define K3BPATTERNPARSER_H

#include <qstring.h>

#include "../k3bcddb.h"


/**
  *@author Sebastian Trueg
  */
class K3bPatternParser 
{
 public: 
  static QString parsePattern( const K3bCddbEntry& entry, 
			       unsigned int trackNumber,
			       const QString& pattern, 
			       bool replace = false, 
			       const QString& replaceString = "_" );
};

#endif
