/* 
 *
 * $Id$
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


#include "k3bcdtextvalidator.h"

K3bCdTextValidator::K3bCdTextValidator(QObject *parent, const char *name)
  : QValidator(parent, name)
{
}


K3bCdTextValidator::~K3bCdTextValidator()
{
}


QValidator::State K3bCdTextValidator::validate( QString& input, int& ) const
{
  if( input.length() > 160 )
    return Invalid;

  for( unsigned int i = 0; i < input.length(); ++i ) {
    // CdText only allows ASCII
    if( input[i].latin1() == 0 )
      return Invalid;
    // forbid some characters that might introduce problems
    if( input[i] == '/' || input[i] == '"' || input[i] == '\\' )
      return Invalid;
  }

  return Acceptable;
}
