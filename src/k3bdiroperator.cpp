/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdiroperator.h"

#include "k3bapplication.h"
#include "k3b.h"
#include "k3bcore.h"
#include "k3baction.h"

#include <QDir>

#include <KAction>
#include <KActionCollection>
#include <KBookmarkMenu>
#include <KConfigGroup>
#include <KMenu>
#include <KStandardDirs>

K3b::DirOperator::DirOperator(const KUrl& url, QWidget* parent )
    : KDirOperator( url, parent )
{
    setMode( KFile::Files );

    // disable the del-key since we still have a focus problem and users keep
    // deleting files when they want to remove project entries
    QAction* aTrash = actionCollection()->action("trash");
    if( aTrash ) {
        aTrash->setShortcut( 0 );
    }

    // add the bookmark stuff

    QString bookmarksFile = KStandardDirs::locateLocal("data", QString::fromLatin1("k3b/bookmarks.xml"));
    KBookmarkManager* bmMan = KBookmarkManager::managerForFile( bookmarksFile, "k3b" );
    bmMan->setEditorOptions( i18n("K3b Bookmarks"), false );
    bmMan->setUpdate( true );

    m_bmPopup = new KActionMenu( KIcon("bookmarks"),i18n("Bookmarks"), this);
    m_bmPopup->setDelayed( false );
    m_bmMenu = new KBookmarkMenu( bmMan, this, m_bmPopup->menu(), actionCollection() );

    (void)K3b::createAction( this,i18n("&Add to Project"), 0, Qt::SHIFT+Qt::Key_Return,
                             this, SLOT(slotAddFilesToProject()),
                             actionCollection(), "add_file_to_project");

    connect( this, SIGNAL(fileSelected(KFileItem)),
             this, SLOT(slotAddFilesToProject()) );
}


K3b::DirOperator::~DirOperator()
{
    delete m_bmMenu;
}


void K3b::DirOperator::readConfig( const KConfigGroup& grp )
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


void K3b::DirOperator::writeConfig( KConfigGroup& grp )
{
    KDirOperator::writeConfig(grp );
    grp.writePathEntry( "last url", url().path() );
}


void K3b::DirOperator::openBookmark(const KBookmark & bm, Qt::MouseButtons, Qt::KeyboardModifiers)
{
    setUrl( bm.url(), true );
}


QString K3b::DirOperator::currentTitle() const
{
    const KUrl& u = url();
    if (u.isLocalFile()) {
        return u.path( KUrl::RemoveTrailingSlash );
    } else {
        return u.prettyUrl();
    }
}


QString K3b::DirOperator::currentUrl() const
{
    return url().prettyUrl();
}


void K3b::DirOperator::activatedMenu( const KFileItem&, const QPoint& pos )
{
    // both from KDirOperator
    setupMenu();
    updateSelectionDependentActions();

    // insert our own actions
    KActionMenu* dirOpMenu = qobject_cast<KActionMenu*>( actionCollection()->action("popupMenu") );
    if (!dirOpMenu) {
        return;
    }
    QAction* firstAction = dirOpMenu->menu()->actions().first();
    dirOpMenu->insertAction( firstAction, actionCollection()->action("add_file_to_project") );
    dirOpMenu->insertSeparator( firstAction );
    dirOpMenu->addSeparator();
    dirOpMenu->addAction( m_bmPopup );

    bool hasSelection = !selectedItems().isEmpty();
    /*
      view() && view()->selectedItems() &&
      !view()->selectedItems()->isEmpty();
    */
    actionCollection()->action("add_file_to_project")->setEnabled( hasSelection && k3bappcore->k3bMainWindow()->activeView() != 0 );

    dirOpMenu->menu()->popup( pos );
}


void K3b::DirOperator::slotAddFilesToProject()
{
    KUrl::List files;
    QList<KFileItem> items(selectedItems());
    Q_FOREACH( const KFileItem& fileItem, items ) {
        files.append( fileItem.url() );
    }
    if( !files.isEmpty() ) {
        k3bappcore->k3bMainWindow()->addUrls( files );
    }
}

#include "k3bdiroperator.moc"

