/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


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
