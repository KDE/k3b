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


#include "k3bdiroperator.h"
#include "kdndfileview.h"

#include <kcombiview.h>
#include <kfilepreview.h>
#include <kaction.h>


K3bDirOperator::K3bDirOperator(const KURL& url, QWidget* parent, const char* name )
  : KDirOperator( url, parent, name )
{
//   // add view-switching actions (no need in KDE 3.1)
//   KAction* detailedViewAction = actionCollection()->action("detailed view");
//   KAction* shortViewAction = actionCollection()->action("short view");

//   KActionMenu* viewMenu = (KActionMenu*)actionCollection()->action("view menu");
//   viewMenu->insert( detailedViewAction, 0 );
//   viewMenu->insert( shortViewAction, 1 );
//   viewMenu->insert( new KActionSeparator( actionCollection() ), 2 );

  // disable the del-key since we still have a focus problem and users keep
  // deleting files when they want to remove project entries
  KAction* aDelete = actionCollection()->action("delete");
  if( aDelete )
    aDelete->setAccel( 0 );
}


K3bDirOperator::~K3bDirOperator()
{
}


KFileView* K3bDirOperator::createView( QWidget* parent, KFile::FileView view )
{
  KFileView* new_view = 0L;
  bool separateDirs = KFile::isSeparateDirs( view );
  bool preview=( (view & KFile::PreviewInfo) == KFile::PreviewInfo ||
		 (view & KFile::PreviewContents) == KFile::PreviewContents );
  
  if( separateDirs ) {
    KCombiView *combi = new KCombiView( parent, "combi view" );
    combi->setOnlyDoubleClickSelectsFiles( onlyDoubleClickSelectsFiles() );
    KFileView* v = 0L;
    if ( (view & KFile::Simple) == KFile::Simple )
      v = createView( combi, KFile::Simple );
    else
      v = createView( combi, KFile::Detail );
    combi->setRight( v );
    new_view = combi;
  }
  else if( (view & KFile::Detail) == KFile::Detail && !preview ) {
    new_view = new KDndFileDetailView( parent, "detail view");
    connect( (KDndFileDetailView*)new_view, SIGNAL(doubleClicked(QListViewItem*)), 
	     this, SLOT(slotListViewItemDoubleClicked(QListViewItem*)) );
  }
  else if ((view & KFile::Simple) == KFile::Simple && !preview ) {
    new_view = new KDndFileIconView( parent, "simple view");
    new_view->setViewName( i18n("Short View") );
    connect( (KDndFileIconView*)new_view, SIGNAL(doubleClicked(QIconViewItem*)), 
	     this, SLOT(slotIconViewItemDoubleClicked(QIconViewItem*)) );
  }
  else { // preview
    KFileView* v = 0L; // will get reparented by KFilePreview
    if ( (view & KFile::Simple ) == KFile::Simple )
      v = createView( 0L, KFile::Simple );
    else
      v = createView( 0L, KFile::Detail );
    
    KFilePreview* pView = new KFilePreview( v, parent, "preview" );
    pView->setOnlyDoubleClickSelectsFiles( onlyDoubleClickSelectsFiles() );
    new_view = pView;
  }

  return new_view;
}


void K3bDirOperator::slotIconViewItemDoubleClicked( QIconViewItem* item )
{
  if( KFileIconViewItem* f = dynamic_cast<KFileIconViewItem*>( item ) )
    if( f->fileInfo()->isFile() )
      emit doubleClicked( f->fileInfo() );
}


void K3bDirOperator::slotListViewItemDoubleClicked( QListViewItem* item )
{
  if( KFileListViewItem* f = dynamic_cast<KFileListViewItem*>( item ) )
    if( f->fileInfo()->isFile() )
      emit doubleClicked( f->fileInfo() );
}


#include "k3bdiroperator.moc"

