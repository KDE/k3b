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


#include "k3bfileview.h"
#include "k3b.h"
#include "k3baudioplayer.h"
#include "k3bdiroperator.h"
#include "k3btoolbox.h"

#include <qwidget.h>
#include <qdragobject.h>
#include <qlayout.h>
#include <qdir.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qtoolbutton.h>

#include <kfiledetailview.h>
#include <klistview.h>
#include <kaction.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kfilefiltercombo.h>
#include <klocale.h>
#include <kfileitem.h>
#include <kmessagebox.h>
#include <kdirlister.h>



K3bFileView::K3bFileView(QWidget *parent, const char *name ) 
  : K3bCdContentsView(false, parent,name) 
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
  QVBoxLayout* layout = new QVBoxLayout( this );
  //  layout->setAutoAdd( true );

  m_dirOp = new K3bDirOperator( QDir::home().absPath(), this );
  m_dirOp->readConfig( k3bMain()->config(), "file view" );
  m_dirOp->setMode( KFile::Files );
  m_dirOp->setView( KFile::Default );

  m_toolBox = new K3bToolBox( this );

  layout->addWidget( m_dirOp );
  layout->addWidget( m_toolBox );
  layout->setStretchFactor( m_dirOp, 1 );


  // setup actions
  KAction* actionPlay = new KAction( i18n("Play"), "player_play", 0, this, SLOT(slotAudioFilePlay()), 
				     m_dirOp->actionCollection(), "audio_file_play");
  KAction* actionEnqueue = new KAction( i18n("En&queue"), "player_play", 0, this, SLOT(slotAudioFileEnqueue()), 
					m_dirOp->actionCollection(), "audio_file_enqueue");
  KAction* actionAddFilesToProject = new KAction( i18n("&Add to Project"), SHIFT+Key_Return, 
						  this, SLOT(slotAddFilesToProject()), 
						  m_dirOp->actionCollection(), "add_file_to_project");

  KAction* actionHome = m_dirOp->actionCollection()->action("home");
  KAction* actionBack = m_dirOp->actionCollection()->action("back");
  KAction* actionUp = m_dirOp->actionCollection()->action("up");
  KAction* actionReload = m_dirOp->actionCollection()->action("reload");


  m_toolBox->addButton( actionUp );
  m_toolBox->addButton( actionBack );
  m_toolBox->addButton( actionHome );
  m_toolBox->addButton( actionReload );
  m_toolBox->addSpacing();
  m_toolBox->addButton( actionPlay );
  m_toolBox->addSpacing();


  // insert actions into diroperator menu
  KActionMenu* dirOpMenu = (KActionMenu*)m_dirOp->actionCollection()->action("popupMenu");
  dirOpMenu->insert( actionAddFilesToProject, 0 );
  dirOpMenu->insert( new KActionSeparator( m_dirOp->actionCollection() ), 1 );
  dirOpMenu->insert( actionPlay, 2 );
  dirOpMenu->insert( actionEnqueue, 3 );
  dirOpMenu->insert( new KActionSeparator( m_dirOp->actionCollection() ), 4 );

  // check if some actions should be enabled
  connect( dirOpMenu, SIGNAL(activated()), this, SLOT(slotCheckActions()) );

  // create filter selection combobox
  m_toolBox->addLabel( i18n("Filter:") );
  m_filterWidget = new KFileFilterCombo( m_toolBox, "filterwidget" );
  m_toolBox->addWidget( m_filterWidget );

  m_filterWidget->setEditable( true );
  QString filter = i18n("*|All Files");
  filter += "\n" + i18n("audio/x-mp3 application/x-ogg audio/x-wav |Sound Files");
  filter += "\n" + i18n("audio/x-wav |Wave Sound Files");
  filter += "\n" + i18n("audio/x-mp3 |MP3 Sound Files");
  filter += "\n" + i18n("application/x-ogg |Ogg Vorbis Sound Files");
  filter += "\n" + i18n("video/mpeg |MPEG Video Files");
  m_filterWidget->setFilter(filter);

  connect( m_filterWidget, SIGNAL(filterChanged()), SLOT(slotFilterChanged()) );

  connect( m_dirOp, SIGNAL(fileHighlighted(const KFileItem*)), this, SLOT(slotFileHighlighted(const KFileItem*)) );
  connect( m_dirOp, SIGNAL(urlEntered(const KURL&)), this, SIGNAL(urlEntered(const KURL&)) );
  connect( m_dirOp, SIGNAL(doubleClicked(KFileItem*)), this, SLOT(slotAddFilesToProject()) );

  slotFileHighlighted(0);
}

void K3bFileView::setDir( const QString& dir )
{
  KURL url;
  url.setPath(dir);
  setUrl( url );
}


void K3bFileView::setUrl(const KURL& url, bool forward)
{
  m_dirOp->setURL( url, forward );
}

KURL K3bFileView::url()
{
  return m_dirOp->url();
}

void K3bFileView::setAutoUpdate( bool b )
{
  m_dirOp->dirLister()->setAutoUpdate( b );
}

void K3bFileView::slotFileHighlighted( const KFileItem* )
{
  // check if there are audio files under the selected ones
  bool play = false;
  for( QPtrListIterator<KFileItem> it( *(m_dirOp->selectedItems()) ); it.current(); ++it ) {
    if( k3bMain()->audioPlayer()->supportsMimetype(it.current()->mimetype()) ) {
      play = true;
      break;
    }
  }

  if( play ) {
    m_dirOp->actionCollection()->action( "audio_file_play" )->setEnabled( true );
    m_dirOp->actionCollection()->action( "audio_file_enqueue" )->setEnabled( true );
  }
  else {
    m_dirOp->actionCollection()->action( "audio_file_play" )->setEnabled( false );
    m_dirOp->actionCollection()->action( "audio_file_enqueue" )->setEnabled( false );
  }
}


void K3bFileView::slotAudioFilePlay()
{
  // play selected audio files
  QStringList files;

  for( QPtrListIterator<KFileItem> it( *(m_dirOp->selectedItems()) ); it.current(); ++it ) {
    if( k3bMain()->audioPlayer()->supportsMimetype(it.current()->mimetype()) ) {
      files.append( it.current()->url().path() );
    }
  }

  if( !files.isEmpty() ) {
    if( !k3bMain()->audioPlayer()->isVisible() )
      k3bMain()->slotViewAudioPlayer();
    k3bMain()->audioPlayer()->playFiles( files );
  }
}


void K3bFileView::slotAudioFileEnqueue()
{
  // play selected audio files
  QStringList files;

  for( QPtrListIterator<KFileItem> it( *(m_dirOp->selectedItems()) ); it.current(); ++it ) {
    if( k3bMain()->audioPlayer()->supportsMimetype(it.current()->mimetype()) ) {
      files.append( it.current()->url().path() );
    }
  }

  if( !files.isEmpty() )
    k3bMain()->audioPlayer()->enqueueFiles( files );
}


void K3bFileView::slotAddFilesToProject()
{
  KURL::List files;
  for( QPtrListIterator<KFileItem> it( *(m_dirOp->selectedItems()) ); it.current(); ++it ) {
    files.append( it.current()->url() );
  }    
  if( !files.isEmpty() )
    k3bMain()->addUrls( files );
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


void K3bFileView::slotCheckActions()
{
  m_dirOp->actionCollection()->action("add_file_to_project")->setEnabled( k3bMain()->activeView() != 0 );
}


void K3bFileView::reload()
{
  m_dirOp->actionCollection()->action("reload")->activate();
}


void K3bFileView::saveConfig( KConfig* c )
{
  m_dirOp->writeConfig( c, "file view" );
}


#include "k3bfileview.moc"
