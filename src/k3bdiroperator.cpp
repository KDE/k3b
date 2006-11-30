
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

#include <k3bapplication.h>
#include <k3b.h>
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
  setMode( KFile::Files );

  // disable the del-key since we still have a focus problem and users keep
  // deleting files when they want to remove project entries
  KAction* aDelete = actionCollection()->action("delete");
  if( aDelete )
    aDelete->setShortcut( 0 );

  // add the bookmark stuff
  KBookmarkManager* bmMan = KBookmarkManager::managerForFile( locateLocal( "data", "k3b/bookmarks.xml" ), false );
  bmMan->setEditorOptions( i18n("K3b Bookmarks"), false );
  bmMan->setUpdate( true );
  bmMan->setShowNSBookmarks( false );

  m_bmPopup = new KActionMenu( i18n("Bookmarks"), "bookmark", this, "bookmarks" );
  m_bmMenu = new KBookmarkMenu( bmMan, this, m_bmPopup->popupMenu(), actionCollection(), true );

  (void)new KAction( i18n("&Add to Project"), SHIFT+Key_Return, 
		     this, SLOT(slotAddFilesToProject()), 
		     actionCollection(), "add_file_to_project");
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
  setView( KFile::Default );

  //
  // There seems to be a bug in the KDELibs which makes setURL crash on
  // some systems when used with a non-existing url
  //
  QString lastUrl = cfg->readPathEntry( "last url", QDir::home().absPath() );
  while( !QFile::exists(lastUrl) ) {
    QString urlUp = lastUrl.section( '/', 0, -2 );
    if( urlUp == lastUrl )
      lastUrl = QDir::home().absPath();
    else
      lastUrl = urlUp;
  }

  setURL( KURL::fromPathOrURL(lastUrl), true );

  cfg->setGroup( oldGroup );

  emit urlEntered( url() );
}


void K3bDirOperator::writeConfig( KConfig* cfg, const QString& group )
{
  QString oldGroup = cfg->group();
  cfg->setGroup( group );

  KDirOperator::writeConfig( cfg, group );
  cfg->writePathEntry( "last url", url().path() );

  cfg->setGroup( oldGroup );
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


void K3bDirOperator::activatedMenu( const KFileItem*, const QPoint& pos )
{
  // both from KDirOperator
  setupMenu();
  updateSelectionDependentActions();

  // insert our own actions
  KActionMenu* dirOpMenu = (KActionMenu*)actionCollection()->action("popupMenu");
  dirOpMenu->insert( new KActionSeparator( actionCollection() ) );
  dirOpMenu->insert( m_bmPopup );

  dirOpMenu->insert( actionCollection()->action("add_file_to_project"), 0 );
  dirOpMenu->insert( new KActionSeparator( actionCollection() ), 1 );

  bool hasSelection = view() && view()->selectedItems() &&
                      !view()->selectedItems()->isEmpty();
  actionCollection()->action("add_file_to_project")->setEnabled( hasSelection && k3bappcore->k3bMainWindow()->activeView() != 0 );

  dirOpMenu->popup( pos );
}


void K3bDirOperator::slotAddFilesToProject()
{
  KURL::List files;
  for( QPtrListIterator<KFileItem> it( *(selectedItems()) ); it.current(); ++it ) {
    files.append( it.current()->url() );
  }    
  if( !files.isEmpty() )
    k3bappcore->k3bMainWindow()->addUrls( files );
}

#include "k3bdiroperator.moc"

