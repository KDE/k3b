/***************************************************************************
                          kdndfiledetailview.cpp  -  description
                             -------------------
    begin                : Sat Apr 20 2002
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

#include "kdndfileview.h"

#include <kurldrag.h>



KDndFileDetailView::KDndFileDetailView( QWidget* parent, const char* name )
  : KFileDetailView( parent, name )
{
  setDragEnabled( true );
}


KDndFileDetailView::~KDndFileDetailView()
{
}


QDragObject* KDndFileDetailView::dragObject()
{
  const KFileItemList* list = KFileView::selectedItems();
  if( list->isEmpty() )
    return 0;

  QListIterator<KFileItem> it(*list);
  KURL::List urls;
	
  for( ; it.current(); ++it )
    urls.append( it.current()->url() );

  return KURLDrag::newDrag( urls, viewport() );
}






KDndFileIconView::KDndFileIconView( QWidget* parent, const char* name )
  : KFileIconView( parent, name )
{
}


KDndFileIconView::~KDndFileIconView()
{
}


QDragObject* KDndFileIconView::dragObject()
{
  const KFileItemList* list = KFileView::selectedItems();
  if( list->isEmpty() )
    return 0;

  QListIterator<KFileItem> it(*list);
  KURL::List urls;
	
  for( ; it.current(); ++it )
    urls.append( it.current()->url() );

  return KURLDrag::newDrag( urls, viewport() );
}



//#include "kdndfileview.moc"
