/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bvalidators.h"


K3bValidator::K3bValidator( QObject* parent, const char* name )
  : QRegExpValidator( parent, name ),
    m_replaceChar('_')
{
}


K3bValidator::K3bValidator( const QRegExp& rx, QObject* parent, const char* name )
  : QRegExpValidator( rx, parent, name ),
    m_replaceChar('_')
{
}


void K3bValidator::fixup( QString& input ) const
{
  for( unsigned int i = 0; i < input.length(); ++i )
    if( !regExp().exactMatch( input.mid(i, 1) ) )
      input[i] = m_replaceChar;
}


QString K3bValidators::fixup( const QString& input, const QRegExp& rx, const QChar& replaceChar )
{
  QString s;
  for( unsigned int i = 0; i < input.length(); ++i )
    if( rx.exactMatch( input.mid(i, 1) ) )
      s += input[i];
    else
      s += replaceChar;
  return s;
}


QRegExp K3bValidators::cdTextCharSet()
{
  return QRegExp("^[^\"/]*$");
}


K3bValidator* K3bValidators::isrcValidator( QObject* parent, const char* name )
{
  return new K3bValidator( QRegExp("^[A-Z\\d]{2,2}-[A-Z\\d]{3,3}-\\d{2,2}-\\d{5,5}$"), parent, name );
}


K3bValidator* K3bValidators::cdTextValidator( QObject* parent, const char* name )
{
  return new K3bValidator( cdTextCharSet(), parent, name );
}


K3bValidator* K3bValidators::iso9660Validator( bool allowEmpty, QObject* parent, const char* name )
{
  if( allowEmpty )
    return new K3bValidator( QRegExp( "[^/]*" ), parent, name );
  else
    return new K3bValidator( QRegExp( "[^/]+" ), parent, name );
}


K3bValidator* K3bValidators::iso646Validator( int type, bool AllowLowerCase, QObject* parent, const char* name )
{
  QRegExp rx;
  switch ( type ) {
  case Iso646_d:
    if ( AllowLowerCase )
      rx = QRegExp( "[a-zA-Z0-9_]*" );
    else
      rx = QRegExp( "[A-Z0-9_]*" );
    break;
  case Iso646_a:
  default: 
    if ( AllowLowerCase )
      rx = QRegExp( "[a-zA-Z0-9!\"\\s%&'\\(\\)\\*\\+,\\-\\./:;<=>\\?_]*" );
    else
      rx = QRegExp( "[A-Z0-9!\"\\s%&'\\(\\)\\*\\+,\\-\\./:;<=>\\?_]*" );
    break;
  }

  return new K3bValidator( rx, parent, name );
}
