/***************************************************************************
                          k3bsetup2task.h
                                   -
                       A K3bSetup task
                             -------------------
    begin                : Sun Aug 25 13:19:59 CEST 2002
    copyright            : (C) 2002 by Sebastian Trueg
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


#include "k3bsetup2task.h"

#include <kiconloader.h>
#include <klocale.h>


K3bSetup2Task::K3bSetup2Task( const QString& text, K3bListView* parent )
  : K3bListViewItem( parent, parent->lastItem(), text )
{
  setText(1, i18n("pending") );
}


void K3bSetup2Task::setHelp( const QString& t )
{
  m_help = t;
  if( !t.isEmpty() )
    setButton( 1, true );
}


void K3bSetup2Task::setFinished( bool success, const QString& errorText )
{
  if( success )
    setPixmap( 1, SmallIcon("ok") );
  else {
    setPixmap( 1, SmallIcon("stop") );
    setText( 1, errorText );
  }
}
