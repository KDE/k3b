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

#ifndef _K3B_VALIDATORS_H_
#define _K3B_VALIDATORS_H_

#include <qvalidator.h>


namespace K3bValidators
{
  /**
   * Validates an ISRC code of the form "CCOOOYYSSSSS" where:
   * <ul>
   * <li>C: country code (upper case letters or digits)</li>
   * <li>O: owner code (upper case letters or digits)</li>
   * <li>Y: year (digits)</li>
   * <li>S: serial number (digits)</li>
   * </ul>
   */
  QValidator* isrcValidator( QObject* parent = 0, const char* name = 0 );
  
  /**
   * FIXME: for now this only forbids ".
   * What is the CDText charset?
   */
  QValidator* cdTextValidator( QObject* parent = 0, const char* name = 0 );

  /**
   * This needs to be replaced by something better in the future...
   * Even the name sucks!
   */
  QValidator* iso9660Validator( bool allowEmpty = true, QObject* parent = 0, const char* name = 0 );

  /**
   * (1) d-characters are: A-Z, 0-9, _ (see ISO-9660:1988, Annex A, Table 15)
   * (2) a-characters are: A-Z, 0-9, _, space, !, ", %, &, ', (, ), *, +, ,, -, ., /, :, ;, <, =, >, ? 
   * (see ISO-9660:1988, Annex A, Table 14)
   */
  enum Iso646Type {
    Iso646_a, 
    Iso646_d 
  };

  QValidator* iso646Validator( int type = Iso646_a, 
			       bool AllowLowerCase = false, 
			       QObject* parent = 0, const char* name = 0 );
}

#endif
