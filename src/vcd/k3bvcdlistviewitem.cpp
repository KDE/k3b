/***************************************************************************
                          k3bvcdlistviewitem.cpp  -  description
                             -------------------
    begin                : Sam Nov 9 2002
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
// QT-includes

#include "k3bvcdlistviewitem.h"
#include "k3bvcdtrack.h"
#include "../tools/k3bglobals.h"

#include <kio/global.h>


K3bVcdListViewItem::K3bVcdListViewItem( K3bVcdTrack* track, K3bListView* parent )
  : K3bListViewItem( parent ), m_track(track)
{
  init();
}

K3bVcdListViewItem::K3bVcdListViewItem( K3bVcdTrack* track, K3bListView* parent, QListViewItem* after )
  : K3bListViewItem( parent, after ), m_track(track)
{
  init();
}


void K3bVcdListViewItem::init()
{
  setEditor( 1, LINE );
}


K3bVcdListViewItem::~K3bVcdListViewItem()
{
}

QString K3bVcdListViewItem::text(int i) const
{
  switch( i )
    {
    case 0:
      return QString::number( m_track->index() +1 ).rightJustify( 2, '0' );
    case 1:
      return m_track->fileName();
    case 2:
      // track mimetype
      return m_track->mimeComment();
    case 3:
      // track size
      return KIO::convertSize( m_track->size() );

    default:
      return KListViewItem::text(i);
    }
}

/*
QString K3bVcdListViewItem::formatSize( unsigned long size) const {
  double dSize = size;
  if ( dSize >= 1024.0*1024.0) {
    return QString("%1 MB").arg(dSize/(1024.0*1024.0),0,'f',2);
  }
  if ( dSize >= 1024.0) {
    return QString("%1 KB").arg(dSize/(1024.0),0,'f',2);
  }
  return QString("%1 B").arg(size);
}
*/
  
void K3bVcdListViewItem::setText(int col, const QString& text )
{
  KListViewItem::setText( col, text );
}


QString K3bVcdListViewItem::key( int, bool ) const
{
  QString num = QString::number( m_track->index() );
  if( num.length() == 1 )
    return "00" + num;
  else if( num.length() == 2 )
    return "0" + num;

  return num;
}
