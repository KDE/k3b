/***************************************************************************
                          k3bmsfedit.h  -  
         An edit widget for MSF (minute,second,frame) values
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg
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
