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

#include <k3bcore.h>

#include <kcombiview.h>
#include <kfilepreview.h>
#include <kaction.h>
#include <kbookmarkmenu.h>
#include <kstandarddirs.h>
#include <kpopupmenu.h>

#include <qdir.h>


K3bDirOperator::K3bDirOperator(const KURL& url, QWidget* parent, const char* name )
  : KDirOperator( url, parent, name )
{
  setViewConfig( k3bcore->config(), "file view" );
  readConfig( k3bcore->config(), "file view" );
  setMode( KFile::Files );
  setView( KFile::Default );

  // disable the del-key since we still have a focus problem and users keep
  // deleting files when they want to remove project entries
  KAction* aDelete = actionCollection()->action("delete");
  if( aDelete )
    aDelete->setAccel( 0 );

  // add the bookmark stuff
  KBookmarkManager* bmMan = KBookmarkManager::managerForFile( locateLocal( "data", "k3b/bookmarks.xml" ), false );
  bmMan->setEditorOptions( i18n("K3b Bookmarks"), false );
  bmMan->setUpdate( true );
  bmMan->setShowNSBookmarks( false );

  m_bmPopup = new KActionMenu( i18n("Bookmarks"), "bookmark", this, "bookmarks" );
  KActionMenu* dirOpMenu = (KActionMenu*)actionCollection()->action("popupMenu");
  dirOpMenu->insert( new KActionSeparator( actionCollection() ) );
  dirOpMenu->insert( m_bmPopup );
  m_bmMenu = new KBookmarkMenu( bmMan, this, m_bmPopup->popupMenu(), actionCollection(), true );
}


K3bDirOperator::~K3bDirOperator()
{
  delete m_bmMenu; 
}


void K3bDirOperator::readConfig( KConfig* cfg, const QString& group )
{
  QString oldGroup = cfg->group();
  cfg->setGroup( group );

  KDirOperator::readConfig( cfg, group );

  //
  // There seems to be a bug in the KDELibs which makes setURL crash on
  // some systems when used with a non-existing url
  //
  QString lastUrl = cfg->readPathEntry( "last url", QDir::home().absPath() );
  if( !QFile::exists(lastUrl) )
    lastUrl = QDir::home().absPath();
  setURL( KURL::fromPathOrURL(lastUrl), true );

  cfg->setGroup( oldGroup );
}


void K3bDirOperator::writeConfig( KConfig* cfg, const QString& group )
{
  QString oldGroup = cfg->group();
  cfg->setGroup( group );

  KDirOperator::writeConfig( cfg, group );
  cfg->writePathEntry( "last url", url().path() );

  cfg->setGroup( oldGroup );
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


void K3bDirOperator::openBookmarkURL( const QString& url )
{
  setURL( KURL::fromPathOrURL( url ), true );
}


QString K3bDirOperator::currentTitle() const
{
  return url().path(-1);
}


QString K3bDirOperator::currentURL() const
{
  return url().path(-1);
}


void K3bDirOperator::activatedMenu( const KFileItem* item, const QPoint& pos )
{
  // TODO: use our own menu and remove or add play and stuff
  return KDirOperator::activatedMenu( item, pos );
}


#include "k3bdiroperator.moc"

