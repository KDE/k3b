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



#include "k3bmsfedit.h"
#include "k3bglobals.h"

#include <qstringlist.h>
#include <qlineedit.h>



K3bMsfValidator::K3bMsfValidator( QObject* parent, const char* name )
  : QRegExpValidator( K3b::Msf::regExp(), parent, name )
{
}



K3bMsfEdit::K3bMsfEdit( QWidget* parent, const char* name )
  : QSpinBox( parent, name )
{
  setValidator( new K3bMsfValidator( this ) );
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
  return K3b::Msf::fromString( text(), ok ).lba();
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

void K3bMsfEdit::setValue( int v )
{
  int i = editor()->cursorPosition();
  QSpinBox::setValue( v );
  editor()->setCursorPosition( i );
}

void K3bMsfEdit::stepUp()
{
  setValue( value() + currentStepValue() );
}

void K3bMsfEdit::stepDown()
{
  setValue( value() - currentStepValue() );
}

int K3bMsfEdit::currentStepValue() const
{
  int val = 1;

  // look if we are currently editing minutes or seconds
  QString text = editor()->text();
  if( text.length() == 8 ) {
    text = text.mid( editor()->cursorPosition() );
    int num = text.contains( ':' );
    if( num == 1 )
      val = 75;
    else if( num == 2 )
      val = 60*75;
  }

  return val;
}

#include "k3bmsfedit.moc"
