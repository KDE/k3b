
/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include <k3baction.h>

#include <kaction.h>
#include <kbookmarkmenu.h>
#include <kstandarddirs.h>
#include <kmenu.h>
#include <kactioncollection.h>
#include <qdir.h>
#include <KConfigGroup>

#include <QAbstractItemView>

K3bDirOperator::K3bDirOperator(const KUrl& url, QWidget* parent )
  : KDirOperator( url, parent )
{
  KConfigGroup grp(k3bcore->config(), "file view" );
  setViewConfig( grp );
  setMode( KFile::Files );

  // disable the del-key since we still have a focus problem and users keep
  // deleting files when they want to remove project entries
  QAction* aDelete = actionCollection()->action("delete");
  if( aDelete )
    aDelete->setShortcut( 0 );

  // add the bookmark stuff

  QString bookmarksFile = KStandardDirs::locateLocal("data", QString::fromLatin1("k3b/bookmarks.xml"));
  KBookmarkManager* bmMan = KBookmarkManager::managerForFile( bookmarksFile, "k3b" );
  bmMan->setEditorOptions( i18n("K3b Bookmarks"), false );
  bmMan->setUpdate( true );

  m_bmPopup = new KActionMenu( KIcon("bookmarks"),i18n("Bookmarks"), this);
  m_bmMenu = new KBookmarkMenu( bmMan, this, m_bmPopup->popupMenu(), actionCollection()/*, true*/ );

  (void)K3b::createAction( this,i18n("&Add to Project"), 0, Qt::SHIFT+Qt::Key_Return, 
		     this, SLOT(slotAddFilesToProject()), 
		     actionCollection(), "add_file_to_project");
}


K3bDirOperator::~K3bDirOperator()
{
  delete m_bmMenu; 
}


void K3bDirOperator::readConfig( const KConfigGroup & grp )
{

  KDirOperator::readConfig( grp );
  setView( KFile::Default );

  //
  // There seems to be a bug in the KDELibs which makes setURL crash on
  // some systems when used with a non-existing url
  //
  QString lastUrl = grp.readPathEntry( "last url", QDir::home().absolutePath() );
  while( !QFile::exists(lastUrl) ) {
    QString urlUp = lastUrl.section( '/', 0, -2 );
    if( urlUp == lastUrl )
      lastUrl = QDir::home().absolutePath();
    else
      lastUrl = urlUp;
  }

  setUrl( KUrl(lastUrl), true );

  emit urlEntered( url() );
}


void K3bDirOperator::writeConfig( KConfigGroup &grp )
{
  KDirOperator::writeConfig(grp );
  grp.writePathEntry( "last url", url().path() );
}


void K3bDirOperator::openBookmarkURL( const QString& url )
{
  setUrl( KUrl( url ), true );
}


QString K3bDirOperator::currentTitle() const
{
  return url().path(KUrl::RemoveTrailingSlash);
}


QString K3bDirOperator::currentURL() const
{
  return url().path(KUrl::RemoveTrailingSlash);
}


void K3bDirOperator::activatedMenu( const KFileItem*, const QPoint& pos )
{
  // both from KDirOperator
  setupMenu();
  updateSelectionDependentActions();

  // insert our own actions
  KActionMenu* dirOpMenu = (KActionMenu*)actionCollection()->action("popupMenu");
  dirOpMenu->addSeparator();
  dirOpMenu->addAction( m_bmPopup );

  dirOpMenu->addAction( actionCollection()->action("add_file_to_project")/*, 0 */);
  dirOpMenu->addSeparator();

  bool hasSelection = !selectedItems().isEmpty();
 /*
 view() && view()->selectedItems() &&
                      !view()->selectedItems()->isEmpty();
  */
  actionCollection()->action("add_file_to_project")->setEnabled( hasSelection && k3bappcore->k3bMainWindow()->activeView() != 0 );

  dirOpMenu->menu()->popup( pos );
}


void K3bDirOperator::slotAddFilesToProject()
{
  KUrl::List files;
  QList<KFileItem> items(selectedItems());
  for( QList<KFileItem>::const_iterator it = items.begin();
       it != items.end(); ++it ) {
    files.append( (*it).url() );
  }    
  if( !files.isEmpty() )
    k3bappcore->k3bMainWindow()->addUrls( files );
}

#include "k3bdiroperator.moc"

