/*
    SPDX-FileCopyrightText: 2003-2007 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "k3bvalidators.h"

#include <ctype.h>


K3b::CharValidator::CharValidator( QObject* parent )
  : QValidator( parent ),
    m_replaceChar( '_' )
{
}


QValidator::State K3b::CharValidator::validate( QString& s, int& pos ) const
{
  Q_UNUSED(pos);

  for( int i = 0; i < s.length(); ++i ) {
    State r = validateChar( s[i] );
    if( r != Acceptable )
      return r;
  }

  return Acceptable;
}


void K3b::CharValidator::fixup( QString& s ) const
{
  for( int i = 0; i < s.length(); ++i ) {
    if( validateChar( s[i] ) != Acceptable )
      s[i] = m_replaceChar;
  }
}


K3b::Latin1Validator::Latin1Validator( QObject* parent )
  : K3b::CharValidator( parent )
{
}


QValidator::State K3b::Latin1Validator::validateChar( const QChar& c ) const
{
  if( !c.toLatin1() )
    return Invalid;
  else
    return Acceptable;
}


K3b::AsciiValidator::AsciiValidator( QObject* parent )
  : K3b::Latin1Validator( parent )
{
}


QValidator::State K3b::AsciiValidator::validateChar( const QChar& c ) const
{
  if( K3b::Latin1Validator::validateChar( c ) == Invalid )
    return Invalid;
  else if( !isascii( c.toLatin1() ) )
    return Invalid;
  else
    return Acceptable;
}



K3b::Validator::Validator( QObject* parent )
  : QRegExpValidator( parent ),
    m_replaceChar('_')
{
}


K3b::Validator::Validator( const QRegExp& rx, QObject* parent )
  : QRegExpValidator( rx, parent ),
    m_replaceChar('_')
{
}


void K3b::Validator::fixup( QString& input ) const
{
  for( int i = 0; i < input.length(); ++i )
    if( !regExp().exactMatch( input.mid(i, 1) ) )
      input[i] = m_replaceChar;
}


QString K3b::Validators::fixup( const QString& input, const QRegExp& rx, const QChar& replaceChar )
{
  QString s;
  for( int i = 0; i < input.length(); ++i )
    if( rx.exactMatch( input.mid(i, 1) ) )
      s += input[i];
    else
      s += replaceChar;
  return s;
}


K3b::Validator* K3b::Validators::isrcValidator( QObject* parent )
{
  return new K3b::Validator( QRegExp("^[A-Z\\d]{2,2}-[A-Z\\d]{3,3}-\\d{2,2}-\\d{5,5}$"), parent );
}


K3b::Validator* K3b::Validators::iso9660Validator( bool allowEmpty, QObject* parent )
{
  if( allowEmpty )
    return new K3b::Validator( QRegExp( "[^/]*" ), parent );
  else
    return new K3b::Validator( QRegExp( "[^/]+" ), parent );
}


K3b::Validator* K3b::Validators::iso646Validator( int type, bool AllowLowerCase, QObject* parent )
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

  return new K3b::Validator( rx, parent );
}
