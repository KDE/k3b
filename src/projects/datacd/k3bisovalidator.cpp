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
