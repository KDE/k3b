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



#include "k3bmsfedit.h"
#include "k3bglobals.h"

#include <qstringlist.h>
#include <qlineedit.h>


K3bMsfEdit::K3bMsfEdit( QWidget* parent, const char* name )
  : QSpinBox( parent, name )
{
  setMinValue( 0 );
  setMaxValue( (60*60*75) + (60*75) + 75 );
}


K3bMsfEdit::~K3bMsfEdit()
{}


QString K3bMsfEdit::mapValueToText( int value )
{
  return K3b::framesToString( value, true );
}


K3b::Msf K3bMsfEdit::msfValue() const
{
  return K3b::Msf(value());
}


void K3bMsfEdit::setMsfValue( const K3b::Msf& msf )
{
  setValue( msf.totalFrames() );
}


int K3bMsfEdit::mapTextToValue( bool* ok )
{
  int m = 0, s = 0, f = 0;

  QStringList tokens = QStringList::split( ":", text() );
  int i = tokens.size()-1;
  if( i >= 0 )
    f = tokens[i].toInt(ok);
  if( !(*ok) )
    return 0;
  --i;
  if( i >= 0 )
    s = tokens[i].toInt(ok);
  if( !(*ok) )
    return 0;
  --i;
  if( i >= 0 )
    m = tokens[i].toInt(ok);
  if( !(*ok) )
    return 0;

  if( f < 0 || s < 0 || m < 0 )
    return 0;

  return f + (s*75) + (m*60*75);
}


void K3bMsfEdit::setText( const QString& str )
{
  bool ok;
  editor()->setText( str );
  setValue( mapTextToValue( &ok) );
}


void K3bMsfEdit::setFrameStyle( int style )
{
  editor()->setFrameStyle( style );
}

void K3bMsfEdit::setLineWidth( int v )
{
  editor()->setLineWidth( v );
}

#include "k3bmsfedit.moc"
