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


#include "k3bfileview.h"
#include "k3b.h"
#include "k3bdiroperator.h"
#include "k3bapplication.h"

#include <qwidget.h>
#include <q3dragobject.h>
#include <qlayout.h>
#include <qdir.h>

#include <qlabel.h>
#include <qtoolbutton.h>
//Added by qt3to4:
#include <Q3VBoxLayout>

#include <k3listview.h>
#include <kaction.h>
#include <ktoolbar.h>
#include <kactioncollection.h>

#include <kurl.h>
#include <k3urldrag.h>
#include <kfilefiltercombo.h>
#include <klocale.h>
#include <kfileitem.h>
#include <kmessagebox.h>
#include <kdirlister.h>
#include <kprogressdialog.h>
#include <ktoolbar.h>
#include <KActionMenu>
#include <kvbox.h>

K3bFileView::K3bFileView(QWidget *parent, const char *name )
  : K3bContentsView( false, parent)
{
  setupGUI();
}


K3bFileView::~K3bFileView()
{
}


KActionCollection* K3bFileView::actionCollection() const
{
  return m_dirOp->actionCollection();
}


void K3bFileView::setupGUI()
{
  Q3VBoxLayout* layout = new Q3VBoxLayout( this );
  //  layout->setAutoAdd( true );

  m_dirOp = new K3bDirOperator( KUrl(QDir::home().absolutePath()), this );
  m_toolBox = new KToolBar( this);

  layout->addWidget( m_toolBox );
  layout->addWidget( m_dirOp );
  layout->setStretchFactor( m_dirOp, 1 );

  // setup actions
  QAction* actionHome = m_dirOp->actionCollection()->action("home");
  QAction* actionBack = m_dirOp->actionCollection()->action("back");
  QAction* actionUp = m_dirOp->actionCollection()->action("up");
  QAction* actionReload = m_dirOp->actionCollection()->action("reload");

  m_toolBox->addAction( actionUp );
  m_toolBox->addAction( actionBack );
  m_toolBox->addAction( actionHome );
  m_toolBox->addAction( actionReload );
  //m_toolBox->addSpacing();
  m_toolBox->addAction( m_dirOp->actionCollection()->action("short view") );
  m_toolBox->addAction( m_dirOp->actionCollection()->action("detailed view") );
  //m_toolBox->addSpacing();
  m_toolBox->addAction( m_dirOp->bookmarkMenu() );
  //m_toolBox->addSpacing();

  // create filter selection combobox
  //m_toolBox->addSpacing();
  m_toolBox->addAction( i18n("Filter:") );
  //m_toolBox->addSpacing();
  m_filterWidget = new KFileFilterCombo( m_toolBox );
  m_toolBox->addWidget( m_filterWidget );
  //m_toolBox->addStretch();
  m_toolBox->addWidget( m_dirOp->progressBar() );

  m_filterWidget->setEditable( true );
  QString filter = i18n("*|All Files");
  filter += "\n" + i18n("audio/x-mp3 audio/x-wav application/x-ogg |Sound Files");
  filter += "\n" + i18n("audio/x-wav |Wave Sound Files");
  filter += "\n" + i18n("audio/x-mp3 |MP3 Sound Files");
  filter += "\n" + i18n("application/x-ogg |Ogg Vorbis Sound Files");
  filter += "\n" + i18n("video/mpeg |MPEG Video Files");
  m_filterWidget->setFilter(filter);

  connect( m_filterWidget, SIGNAL(filterChanged()), SLOT(slotFilterChanged()) );

  connect( m_dirOp, SIGNAL(fileHighlighted(const KFileItem*)), this, SLOT(slotFileHighlighted(const KFileItem*)) );
  connect( m_dirOp, SIGNAL(urlEntered(const KUrl&)), this, SIGNAL(urlEntered(const KUrl&)) );
  connect( m_dirOp, SIGNAL(fileSelected(const KFileItem*)), m_dirOp, SLOT(slotAddFilesToProject()) );

  slotFileHighlighted(0);
}

void K3bFileView::setDir( const QString& dir )
{
  KUrl url;
  url.setPath(dir);
  setUrl( url );
}


void K3bFileView::setUrl(const KUrl& url, bool forward)
{
  m_dirOp->setUrl( url, forward );
}

KUrl K3bFileView::url()
{
  return m_dirOp->url();
}

void K3bFileView::setAutoUpdate( bool b )
{
  m_dirOp->dirLister()->setAutoUpdate( b );
}

void K3bFileView::slotFileHighlighted( const KFileItem* )
{
}


void K3bFileView::slotFilterChanged()
{
  QString filter = m_filterWidget->currentFilter();
  m_dirOp->clearFilter();

  if( filter.find( '/' ) > -1 ) {
    QStringList types = QStringList::split( " ", filter );
    types.prepend( "inode/directory" );
    m_dirOp->setMimeFilter( types );
  }
  else
    m_dirOp->setNameFilter( filter );

  m_dirOp->rereadDir();
  //  emit filterChanged( filter );
}


void K3bFileView::reload()
{
  //FIXME kde4
  m_dirOp->actionCollection()->action("reload")->activate(QAction::Trigger);
}


void K3bFileView::saveConfig( KConfigGroup &grp )
{
  m_dirOp->writeConfig(grp );
}


void K3bFileView::readConfig( const KConfigGroup& grp )
{
  m_dirOp->readConfig( grp);
}

#include "k3bfileview.moc"
