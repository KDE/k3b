/***************************************************************************
                          k3bisovalidator.h  -  description
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

#ifndef K3BISOVALIDATOR_H
#define K3BISOVALIDATOR_H

#include <qvalidator.h>

/**
 * A Validator for all values in a data project
 * @author Sebastian Trueg
 */
class K3bIsoValidator : public QRegExpValidator
{
 public: 
  K3bIsoValidator( QObject* parent = 0, const char* name = 0, bool allowEmpty = true );
  ~K3bIsoValidator();
};

#endif
