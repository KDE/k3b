/***************************************************************************
                          k3bfileview.cpp  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include "k3bfileview.h"
#include "k3b.h"
#include "k3baudioplayer.h"

#include <qwidget.h>
#include <qdragobject.h>
#include <qlayout.h>
#include <qdir.h>
#include <qvbox.h>
#include <qlabel.h>

#include <kfiledetailview.h>
#include <klistview.h>
#include <kaction.h>
#include <kdiroperator.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kurl.h>
#include <kfilefilter.h>
#include <klocale.h>
#include <kfileviewitem.h>



K3bFileView::PrivateFileView::PrivateFileView( QWidget* parent, const char* name )
  : KFileDetailView( parent, name )
{
  setDragEnabled( true );
}

	
QDragObject* K3bFileView::PrivateFileView::dragObject() const
{
  if( !currentItem() )
    return 0;
	
  const KFileViewItemList* list = KFileView::selectedItems();
  QListIterator<KFileViewItem> it(*list);
  QStrList dragstr;
	
  for( ; it.current(); ++it )
    dragstr.append( it.current()->url().path(-1) );
		
  return new QUriDrag( dragstr, viewport() );
}




K3bFileView::K3bFileView(QWidget *parent, const char *name ) 
  : QVBox(parent,name) 
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
  m_dirOp           = new KDirOperator( QDir::home().absPath(), this );
  KToolBar* toolBar = new KToolBar( k3bMain(), this, "fileviewtoolbar" );

  // PrivateFileView just adds d'n'd support (replace with default detailView in KDE3)
  PrivateFileView* fileView = new PrivateFileView( m_dirOp, "fileview" );
  m_dirOp->setView( fileView );
  fileView->setSelectionMode( KFile::Extended );

  KAction* actionPlay = new KAction( i18n("&Play"), "1rightarrow", 0, this, SLOT(slotAudioFilePlay()), 
				     m_dirOp->actionCollection(), "audio_file_play");

  // add some actions to the toolbar
//   m_dirOp->actionCollection()->action("up")->plug( toolBar );
//   m_dirOp->actionCollection()->action("home")->plug( toolBar );
//   m_dirOp->actionCollection()->action("reload")->plug( toolBar );
//   toolBar->insertSeparator();
  actionPlay->plug( toolBar );
  toolBar->insertSeparator();

  KActionMenu* dirOpMenu = (KActionMenu*)m_dirOp->actionCollection()->action("popupMenu");
  dirOpMenu->insert( actionPlay, 0 );
  dirOpMenu->insert( new KActionSeparator( m_dirOp->actionCollection() ), 1 );

  // this has to be disabled since the user must NEVER change the fileview because
  // that would disable the dragging support! (obsolete in KDE3)
  m_dirOp->actionCollection()->action("view menu")->setEnabled( false );

  // create filter selection combobox
  QLabel* filterLabel = new QLabel( i18n("&Filter:"), toolBar, "filterLabel" );
  m_filterWidget = new KFileFilter( toolBar, "filterwidget" );
  m_filterWidget->setEditable( true );
  QString filter = i18n("*|All files");
  filter += "\n" + i18n("audio/x-mp3 audio/x-ogg audio/wav |Sound files");
  filter += "\n" + i18n("audio/x-mp3 |MP3 sound files");
  filter += "\n" + i18n("application/x-ogg |Ogg Vorbis sound files");
  m_filterWidget->setFilter(filter);

  filterLabel->setBuddy(m_filterWidget);
  connect( m_filterWidget, SIGNAL(filterChanged()), SLOT(slotFilterChanged()) );

  connect( m_dirOp, SIGNAL(fileHighlighted(const KFileViewItem*)), this, SLOT(slotFileHighlighted(const KFileViewItem*)) );
  connect( m_dirOp, SIGNAL(urlEntered(const KURL&)), this, SIGNAL(urlEntered(const KURL&)) );
}


void K3bFileView::setUrl(const KURL& url, bool forward)
{
  m_dirOp->setURL( url, forward );
}


void K3bFileView::slotFileHighlighted( const KFileViewItem* item )
{
  // check if there are audio files under the selected ones
  bool play = false;
  for( QListIterator<KFileViewItem> it( *(m_dirOp->selectedItems()) ); it.current(); ++it ) {
    if( k3bMain()->audioPlayer()->supportsMimetype(it.current()->mimetype()) ) {
      play = true;
      break;
    }
  }

  if( play ) {
    m_dirOp->actionCollection()->action( "audio_file_play" )->setEnabled( true );
  }
  else {
    m_dirOp->actionCollection()->action( "audio_file_play" )->setEnabled( false );
  }
}


void K3bFileView::slotAudioFilePlay()
{
  // play selected audio files
  for( QListIterator<KFileViewItem> it( *(m_dirOp->selectedItems()) ); it.current(); ++it ) {
    if( k3bMain()->audioPlayer()->supportsMimetype(it.current()->mimetype()) ) {
      if( k3bMain()->audioPlayer()->playFile( it.current()->url().path() ) ) {
	break;
      }
    }
  }
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


#include "k3bfileview.moc"
