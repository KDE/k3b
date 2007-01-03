/* 
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bvalidators.h"

#include <ctype.h>


K3bCharValidator::K3bCharValidator( QObject* parent, const char* name )
  : QValidator( parent, name ),
    m_replaceChar( '_' )
{
}


QValidator::State K3bCharValidator::validate( QString& s, int& pos ) const
{
  Q_UNUSED(pos);

  for( unsigned int i = 0; i < s.length(); ++i ) {
    State r = validateChar( s[i] );
    if( r != Acceptable )
      return r;
  }

  return Acceptable;
}


void K3bCharValidator::fixup( QString& s ) const
{
  for( unsigned int i = 0; i < s.length(); ++i ) {
    if( validateChar( s[i] ) != Acceptable )
      s[i] = m_replaceChar;
  }
}


K3bLatin1Validator::K3bLatin1Validator( QObject* parent, const char* name )
  : K3bCharValidator( parent, name )
{
}


QValidator::State K3bLatin1Validator::validateChar( const QChar& c ) const
{
  if( !c.latin1() )
    return Invalid;
  else
    return Acceptable;
}


K3bAsciiValidator::K3bAsciiValidator( QObject* parent, const char* name )
  : K3bLatin1Validator( parent, name )
{
}


QValidator::State K3bAsciiValidator::validateChar( const QChar& c ) const
{
  if( K3bLatin1Validator::validateChar( c ) == Invalid )
    return Invalid;
  else if( !isascii( c.latin1() ) )
    return Invalid;
  else
    return Acceptable;
}



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


K3bValidator* K3bValidators::isrcValidator( QObject* parent, const char* name )
{
  return new K3bValidator( QRegExp("^[A-Z\\d]{2,2}-[A-Z\\d]{3,3}-\\d{2,2}-\\d{5,5}$"), parent, name );
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
