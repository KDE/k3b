/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

// kde includes
#include <kaction.h>
#include <kcutlabel.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kstdaction.h>

// qt includes
#include <qfont.h>
#include <qframe.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>

// k3b includes
#include "k3bvideocdview.h"
#include "k3bvideocdrippingdialog.h"
#include <cdinfo/k3bdiskinfodetector.h>
#include <device/k3bmsf.h>
#include <device/k3btoc.h>
#include <k3bcore.h>
#include <k3blistview.h>
#include <k3bstdguiitems.h>
#include <k3btoolbox.h>


class K3bVideoCdView::VideoTrackViewItem  : public QListViewItem
{
public:
  VideoTrackViewItem( QListViewItem* parent,
                      QString name,
		      int _trackNumber,
		      const K3b::Msf& length)
 :QListViewItem( parent ) {
    setText( 0, i18n("%1. %2-%3").arg(_trackNumber).arg(name).arg(_trackNumber) );
    setText( 1, i18n("n/a") );
    setText( 2, length.toString() );
    setText( 3, KIO::convertSize( length.mode2Form2Bytes() ) );

    trackNumber = _trackNumber;
    setSelectable( false );
    // setOn(true);
  }

  int trackNumber;

  void updateData( const K3bVideoCdInfoResultEntry& resultEntry ) {
    setText( 0, QString("%1. %2").arg( trackNumber ).arg( resultEntry.id ) );
    setText( 1, resultEntry.name );
  }

};

class K3bVideoCdView::VideoTrackViewCheckItem : public QCheckListItem
{
public:
  VideoTrackViewCheckItem( QListView* parent,
                   QString desc )
    : QCheckListItem( parent,
		      QString::null,
		      QCheckListItem::CheckBox ) {
    setText( 0, desc );

    setOn(true);
  }

  VideoTrackViewCheckItem( VideoTrackViewCheckItem* parent,
                   QString desc )
    : QCheckListItem( parent,
		      QString::null,
		      QCheckListItem::CheckBox ) {
    setText( 0, desc );
    setOn(true);
  }

  void updateData( const K3b::Msf& length, bool form2=false ) {
    setText( 2, length.toString() );
    if ( form2 )
        setText( 3, KIO::convertSize( length.mode2Form2Bytes() ) );
    else
        setText( 3, KIO::convertSize( length.mode2Form1Bytes() ) );
  }

};

K3bVideoCdView::K3bVideoCdView( QWidget* parent, const char *name )
  : K3bCdContentsView( true, parent, name )
{
  QGridLayout* mainGrid = new QGridLayout( mainWidget() );

  // toolbox
  // ----------------------------------------------------------------------------------
  QHBoxLayout* toolBoxLayout = new QHBoxLayout( 0, 0, 0, "toolBoxLayout" );
  m_toolBox = new K3bToolBox( mainWidget() );
  toolBoxLayout->addWidget( m_toolBox );
  QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
  toolBoxLayout->addItem( spacer );
  m_labelLength = new QLabel( mainWidget() );
  m_labelLength->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  toolBoxLayout->addWidget( m_labelLength );

  // the track view
  // ----------------------------------------------------------------------------------
  m_trackView = new K3bListView( mainWidget() );
  m_trackView->setFullWidth(true);
  m_trackView->setAllColumnsShowFocus( true );
  m_trackView->setSelectionMode( QListView::NoSelection );
  m_trackView->setDragEnabled( true );
  m_trackView->addColumn( i18n("Tracks") );
  m_trackView->addColumn( i18n("Filename") );
  m_trackView->addColumn( i18n("Length") );
  m_trackView->addColumn( i18n("Size") );

  m_trackView->header()->setClickEnabled(false);

  m_trackView->setItemsRenameable(false);
  m_trackView->setRootIsDecorated( true );

  connect( m_trackView, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(slotContextMenu(KListView*, QListViewItem*, const QPoint&)) );
  connect( m_trackView, SIGNAL(selectionChanged(QListViewItem*)),
	   this, SLOT(slotTrackSelectionChanged(QListViewItem*)) );

  mainGrid->addLayout( toolBoxLayout, 0, 0 );
  mainGrid->addWidget( m_trackView, 1, 0 );

  initActions();
  slotTrackSelectionChanged(0);

  m_videocdinfo = 0L;

  m_contentList.clear();
}


K3bVideoCdView::~K3bVideoCdView()
{
  delete m_videocdinfo;
}


void K3bVideoCdView::setDisk( K3bCdDevice::DiskInfoDetector* did )
{
  m_diskInfo = did->diskInfo();

  m_trackView->clear();
  enableInteraction(false);

  m_contentList.append( new VideoTrackViewCheckItem( m_trackView, "Video Tracks" ) );
  m_contentList.append( new VideoTrackViewCheckItem( m_trackView, "ISO Track" ) );

  ((VideoTrackViewCheckItem*)m_contentList[ 0 ])->setOpen( true );

  // create a listviewItem for every video track
  int index = 0;
  m_videocdsize = 0;
  
  for( K3bToc::const_iterator it = m_diskInfo.toc.begin();
       it != m_diskInfo.toc.end(); ++it ) {

    if( index > 0 ) {
      K3b::Msf length( (*it).length() );
      m_videocdsize += (length.mode2Form1Bytes() + 2047) / 2048;
      (void)new VideoTrackViewItem( (VideoTrackViewCheckItem*) m_contentList[ 0 ], "Sequence", index, length );
    }
    else {
        K3b::Msf length( (*it).length() );
        m_videocdsize += (length.mode2Form2Bytes() + 2351) / 2352;
        ((VideoTrackViewCheckItem*)m_contentList[ 1 ])->updateData( length );
        (void)new VideoTrackViewCheckItem( (VideoTrackViewCheckItem*) m_contentList[ 1 ], "Files" );
        (void)new VideoTrackViewCheckItem( (VideoTrackViewCheckItem*) m_contentList[ 1 ], "Segments" );
    }

    index++;
  }

  m_videocdinfo = new K3bVideoCdInfo( this );
  m_videocdinfo->info( m_diskInfo.device->devicename() );

  connect( m_videocdinfo, SIGNAL(infoFinished( bool )),
	   this, SLOT(slotVideoCdInfoFinished( bool )) );
  
}

void K3bVideoCdView::slotVideoCdInfoFinished( bool success )
{

  if( success ) {
    m_videocdinfoResult = m_videocdinfo->result();
    updateDisplay();
  }

  enableInteraction(true);
}

void K3bVideoCdView::updateDisplay()
{
  // update the listview

  VideoTrackViewItem* item = (VideoTrackViewItem*) m_contentList[ 0 ]->firstChild();
  int index = 0;
  while ( item ) {
      item->updateData( m_videocdinfoResult.entry( index, K3bVideoCdInfoResult::SEQUENCE ));
      item = (VideoTrackViewItem*) item->nextSibling();
      index++;
  }

  VideoTrackViewCheckItem* check_item = (VideoTrackViewCheckItem*) m_contentList[ 1 ]->firstChild();
  while ( check_item ) {
      if ( check_item->key( 0, false ).compare("Files") == 0 ) {
      }
      else {
          index = 0;
          for ( index = 0; index < m_videocdinfoResult.foundEntries(K3bVideoCdInfoResult::SEGMENT); index++) {
            (void)new VideoTrackViewItem( check_item, m_videocdinfoResult.entry(index, K3bVideoCdInfoResult::SEGMENT).name ,index, 0 );
          }
      }
      check_item = (VideoTrackViewCheckItem*) check_item->nextSibling();
  }
    
  if( !m_videocdinfoResult.volumeId.isEmpty() )
    setTitle( m_videocdinfoResult.volumeId + " (" + m_videocdinfoResult.type +  " " + m_videocdinfoResult.version + ")");
  else
    setTitle( i18n("Video CD") );

  m_labelLength->setText( i18n("1 track (%1)", "%n tracks (%1)", m_diskInfo.toc.count()).arg(K3b::Msf(m_diskInfo.toc.length()).toString()) );
}


void K3bVideoCdView::initActions()
{
  m_actionCollection = new KActionCollection( this );

  KAction* actionSelectAll = KStdAction::selectAll( this, SLOT(slotSelectAll()),
						    m_actionCollection, "select_all" );
  KAction* actionDeselectAll = KStdAction::deselect( this, SLOT(slotDeselectAll()),
						     m_actionCollection, "deselect_all" );
  actionDeselectAll->setText( i18n("Dese&lect All") );
  KAction* actionSelect = new KAction( i18n("Select Track"), 0, 0, this,
				       SLOT(slotSelect()), actionCollection(),
				       "select_track" );
  KAction* actionDeselect = new KAction( i18n("Deselect Track"), 0, 0, this,
					 SLOT(slotDeselect()), actionCollection(),
					 "deselect_track" );

  KAction* actionStartRip = new KAction( i18n("Start Ripping"), "run", 0, this,
					 SLOT(startRip()), actionCollection(), "start_rip" );

  // TODO: set the actions tooltips and whatsthis infos

  // setup the popup menu
  m_popupMenu = new KActionMenu( actionCollection(), "popup_menu" );
  KAction* separator = new KActionSeparator( actionCollection(), "separator" );
  m_popupMenu->insert( actionSelect );
  m_popupMenu->insert( actionDeselect );
  m_popupMenu->insert( actionSelectAll );
  m_popupMenu->insert( actionDeselectAll );
  m_popupMenu->insert( separator );
  m_popupMenu->insert( actionStartRip );

  // setup the toolbox
  m_toolBox->addButton( actionStartRip );
}


void K3bVideoCdView::slotContextMenu( KListView*, QListViewItem*, const QPoint& p )
{
  m_popupMenu->popup(p);
}


void K3bVideoCdView::slotTrackSelectionChanged( QListViewItem* item )
{
  actionCollection()->action("select_track")->setEnabled( item != 0 );
  actionCollection()->action("deselect_track")->setEnabled( item != 0 );
}


void K3bVideoCdView::startRip()
{
    K3bVideoCdRippingDialog rip( m_diskInfo.device->devicename(), m_videocdsize, this );
    rip.exec();
}

void K3bVideoCdView::slotSelectAll()
{
  for( QListViewItemIterator it( m_trackView ); it.current(); ++it )
    if ( it.current()->isSelectable() )
        ((VideoTrackViewCheckItem*)it.current())->setOn(true);
}

void K3bVideoCdView::slotDeselectAll()
{
  for( QListViewItemIterator it( m_trackView ); it.current(); ++it )
    if ( it.current()->isSelectable() )
        ((VideoTrackViewCheckItem*)it.current())->setOn(false);
}

void K3bVideoCdView::slotSelect()
{
  if( QListViewItem* sel = m_trackView->selectedItem() )
    ((VideoTrackViewCheckItem*)sel)->setOn(true);
}

void K3bVideoCdView::slotDeselect()
{
  if( QListViewItem* sel = m_trackView->selectedItem() )
    ((VideoTrackViewCheckItem*)sel)->setOn(false);
}

void K3bVideoCdView::enableInteraction( bool b )
{
  m_trackView->setEnabled(b);
  m_toolBox->setEnabled(b);
}

#include "k3bvideocdview.moc"
