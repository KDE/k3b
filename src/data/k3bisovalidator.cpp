/***************************************************************************
                          k3bisovalidator.cpp  -  description
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

#include "k3bisovalidator.h"

#include <qregexp.h>


K3bIsoValidator::K3bIsoValidator( QObject* parent, const char* name, bool allowEmpty )
  : QRegExpValidator( parent, name )
{
  if( allowEmpty )
    setRegExp( QRegExp( "[^/$\\\"%]*" ) );
  else
    setRegExp( QRegExp( "[^/$\\\"%]+" ) );
}


K3bIsoValidator::~K3bIsoValidator()
{
}
