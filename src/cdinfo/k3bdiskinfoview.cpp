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


#include "k3bdiskinfoview.h"
#include "k3bdiskinfodetector.h"

#include <k3bdiskinfo.h>
#include <k3bcdtext.h>
#include <k3bdeviceglobals.h>
#include <k3bglobals.h>
#include <k3bstdguiitems.h>
#include <k3blistview.h>
#include <k3biso9660.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qfont.h>
#include <qcolor.h>
#include <qheader.h>
#include <qstring.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpixmap.h>
#include <qregion.h>
#include <qframe.h>

#include <kdialog.h>
#include <klocale.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kio/global.h>



class K3bDiskInfoView::HeaderViewItem : public KListViewItem
{
public:
  HeaderViewItem( QListView* parent )
      : KListViewItem( parent ) {}
  HeaderViewItem( QListViewItem* parent )
      : KListViewItem( parent ) {}
  HeaderViewItem( QListView* parent, QListViewItem* after )
      : KListViewItem( parent, after ) {}
  HeaderViewItem( QListViewItem* parent, QListViewItem* after )
      : KListViewItem( parent, after ) {}
  HeaderViewItem( QListView* parent, const QString& t1 )
      : KListViewItem( parent, t1 ) {}
  HeaderViewItem( QListViewItem* parent, const QString& t1 )
      : KListViewItem( parent, t1 ) {}
  HeaderViewItem( QListView* parent, QListViewItem* after, const QString& t1 )
      : KListViewItem( parent, after, t1 ) {}
  HeaderViewItem( QListViewItem* parent, QListViewItem* after, const QString& t1 )
      : KListViewItem( parent, after, t1 ) {}

  void paintCell( QPainter* p, const QColorGroup & cg, int column, int width, int align )
  {
    QFont f ( p->font() );
    f.setBold( true );
    p->setFont( f );
    KListViewItem::paintCell( p, cg, column, width, align );
  }
};


class K3bDiskInfoView::TwoColumnViewItem : public KListViewItem
{
public:
  TwoColumnViewItem( QListView* parent )
      : KListViewItem( parent ) {}
  TwoColumnViewItem( QListViewItem* parent )
      : KListViewItem( parent ) {}
  TwoColumnViewItem( QListView* parent, QListViewItem* after )
      : KListViewItem( parent, after ) {}
  TwoColumnViewItem( QListViewItem* parent, QListViewItem* after )
      : KListViewItem( parent, after ) {}
  TwoColumnViewItem( QListView* parent, const QString& t1 )
      : KListViewItem( parent, t1 ) {}
  TwoColumnViewItem( QListViewItem* parent, const QString& t1 )
      : KListViewItem( parent, t1 ) {}
  TwoColumnViewItem( QListView* parent, QListViewItem* after, const QString& t1 )
      : KListViewItem( parent, after, t1 ) {}
  TwoColumnViewItem( QListViewItem* parent, QListViewItem* after, const QString& t1 )
      : KListViewItem( parent, after, t1 ) {}

  void paintCell( QPainter* p, const QColorGroup & cg, int column, int width, int align )
  {

    if( column == 1 ) {
      int newWidth = width;

      int i = 2;
      for( ; i < listView()->columns(); ++i ) {
        newWidth += listView()->columnWidth( i );
      }

      // TODO: find a way to get the TRUE new width after resizing

      //       QRect r = p->clipRegion().boundingRect();
      //       r.setWidth( newWidth );
      //       p->setClipRect( r );
      p->setClipping( false );

      KListViewItem::paintCell( p, cg, column, newWidth, align );
    } else if( column == 0 )
      KListViewItem::paintCell( p, cg, column, width, align );
  }
};



K3bDiskInfoView::K3bDiskInfoView( QWidget* parent, const char* name )
  : K3bCdContentsView( true, parent, name )
{
  m_infoView = new KListView( this );
  setMainWidget( m_infoView );

  m_infoView->setSorting( -1 );
  m_infoView->setAllColumnsShowFocus( true );
  m_infoView->setSelectionMode( QListView::NoSelection );
  m_infoView->setResizeMode( KListView::AllColumns );
  m_infoView->setAlternateBackground( QColor() );

  m_infoView->addColumn( "1" );
  m_infoView->addColumn( "2" );
  m_infoView->addColumn( "3" );
  m_infoView->addColumn( "4" );

  m_infoView->header()->hide();
}


K3bDiskInfoView::~K3bDiskInfoView()
{}


void K3bDiskInfoView::displayInfo( const K3bCdDevice::DiskInfo& )
{
  // TODO: drop this method
}


void K3bDiskInfoView::displayInfo( K3bCdDevice::DiskInfoDetector* did )
{
  const K3bCdDevice::DiskInfo& ngInfo = did->diskInfo();
  const K3bCdDevice::Toc& toc = did->toc();

  m_infoView->clear();
  //  m_infoView->header()->resizeSection( 0, 20 );

  if( ngInfo.diskState() == K3bCdDevice::STATE_NO_MEDIA ) {
    (void)new QListViewItem( m_infoView, i18n("No Disk") );
    setTitle( i18n("No disk in drive") );
    setRightPixmap( "diskinfo_right" );
  }
  else {

    if( ngInfo.empty() ) {
      setTitle( i18n("Empty %1 media").arg(K3bCdDevice::mediaTypeString( ngInfo.mediaType(), true )) );
      setRightPixmap( "diskinfo_empty" );
    } 
    else {
      switch( toc.contentType() ) {
      case K3bCdDevice::AUDIO:
        setTitle( i18n("Audio CD") );
	setRightPixmap( "diskinfo_audio" );
        break;
      case K3bCdDevice::DATA:
	if( K3bCdDevice::isDvdMedia( ngInfo.mediaType() ) ) {
	  setTitle( did->isVideoDvd() ? i18n("Video DVD") : i18n("DVD") );
	  setRightPixmap( "diskinfo_dvd" );
	}
	else {
	  setTitle( did->isVideoCd() ? i18n("Video CD") : i18n("Data CD") );
	  setRightPixmap( "diskinfo_data" );
	}
        break;
      case K3bCdDevice::MIXED:
        setTitle( did->isVideoCd() ? i18n("Video CD") : i18n("Mixed mode CD") );
	setRightPixmap( "diskinfo_mixed" );
        break;
      default:
	setTitle( i18n("Unknown disk type") );
	setRightPixmap( "diskinfo_right" );
      }
    }

    createMediaInfoItems( ngInfo );


    // iso9660 info
    // /////////////////////////////////////////////////////////////////////////////////////
    if( did->iso9660() ) {
      (void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item
      createIso9660InfoItems( did->iso9660() );
    }

    // tracks
    // /////////////////////////////////////////////////////////////////////////////////////
    if( !toc.isEmpty() ) {

      if( m_infoView->childCount() )
	(void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item
      
      KListViewItem* trackHeaderItem = new HeaderViewItem( m_infoView, m_infoView->lastChild(), i18n("Tracks") );

      // create header item
      KListViewItem* item = new KListViewItem( trackHeaderItem,
					       i18n("Type"),
					       i18n("Attributes"),
					       i18n("First-Last Sector"),
					       i18n("Length") );

      int lastSession = 0;

      // if we have multible sessions we create a header item for every session
      KListViewItem* trackItem = 0;
      if( ngInfo.numSessions() > 1 && toc[0].session() > 0 ) {
	trackItem = new HeaderViewItem( trackHeaderItem, item, i18n("Session %1").arg(1) );
	lastSession = 1;
      }
      else
	trackItem = trackHeaderItem;

      // create items for the tracks
      K3bToc::const_iterator it;
      int index = 1;
      for( it = toc.begin(); it != toc.end(); ++it ) {
        const K3bTrack& track = *it;

	if( ngInfo.numSessions() > 1 && track.session() != lastSession ) {
	  lastSession = track.session();
	  trackItem->setOpen(true);
	  trackItem = new HeaderViewItem( trackHeaderItem, 
					  m_infoView->lastItem()->parent(), 
					  i18n("Session %1").arg(lastSession) );
	}

        item = new KListViewItem( trackItem, item );
        QString text;
        if( track.type() == K3bTrack::AUDIO ) {
          item->setPixmap( 0, SmallIcon( "sound" ) );
          text = i18n("Audio");

	  // DEBUGGING:
// 	  for( unsigned int i = 0; i <= track.indexCount(); ++i )
// 	    (void)new KListViewItem( item, 
// 				     QString::number( i ),
// 				     QString("%1 (%2)").arg(track.index(i)).arg(K3b::Msf(track.index(i)).toString()) );

// 	  item->setOpen(true);

        } else {
          item->setPixmap( 0, SmallIcon( "tar" ) );
          if( track.mode() == K3bTrack::MODE1 )
            text = i18n("Data/Mode1");
          else if( track.mode() == K3bTrack::MODE2 )
            text = i18n("Data/Mode2");
          else if( track.mode() == K3bTrack::XA_FORM1 )
            text = i18n("Data/Mode2 XA Form1");
          else if( track.mode() == K3bTrack::XA_FORM2 )
            text = i18n("Data/Mode2 XA Form2");
	  else
	    text = i18n("Data");
        }
        item->setText( 0, i18n("%1 (%2)").arg( QString::number(index).rightJustify( 2, ' ' )).arg(text) );
	item->setText( 1, QString( "%1/%2" )
		       .arg( track.copyPermitted() ? i18n("copy") : i18n("no copy") )
		       .arg( track.type() == K3bTrack::AUDIO 
			    ? ( track.preEmphasis() ?  i18n("preemp") : i18n("no preemp") )
			    : ( track.recordedIncremental() ?  i18n("incremental") : i18n("uninterrupted") ) ) );
        item->setText( 2, 
		       QString("%1 - %2")
		       .arg(track.firstSector().lba())
		       .arg(track.lastSector().lba()) );
        item->setText( 3, QString::number( track.length().lba() ) + " (" + track.length().toString() + ")" );
        ++index;
      }

      trackItem->setOpen(true);
      trackHeaderItem->setOpen( true );
    }


    // CD-TEXT
    // /////////////////////////////////////////////////////////////////////////////////////
    if( !did->cdText().isEmpty() ) {
      if( m_infoView->childCount() )
	(void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

      KListViewItem* cdTextHeaderItem = new HeaderViewItem( m_infoView, 
							    m_infoView->lastChild(), 
							    i18n("CD-TEXT (excerpt)") );

      // create header item
      KListViewItem* item = new KListViewItem( cdTextHeaderItem,
					       i18n("Performer"),
					       i18n("Title"),
					       i18n("Songwriter"),
					       i18n("Composer") );
      item = new KListViewItem( cdTextHeaderItem, item );
      item->setText( 0, i18n("CD:") + " " +
		     did->cdText().performer() );
      item->setText( 1, did->cdText().title() );
      item->setText( 2, did->cdText().songwriter() );
      item->setText( 3, did->cdText().composer() );
      
      int index = 1;
      for( unsigned int i = 0; i < did->cdText().count(); ++i ) {
        item = new KListViewItem( cdTextHeaderItem, item );
	item->setText( 0, QString::number(index).rightJustify( 2, ' ' ) + " " +
		       did->cdText().at(i).performer() );
	item->setText( 1, did->cdText().at(i).title() );
	item->setText( 2, did->cdText().at(i).songwriter() );
	item->setText( 3, did->cdText().at(i).composer() );
	++index;
      }

      cdTextHeaderItem->setOpen( true );
    }
  }
}



void K3bDiskInfoView::reload()
{
  //   if( m_currentInfo )
  //     m_diskInfoDetector->detect( m_currentInfo.device );
}


void K3bDiskInfoView::createMediaInfoItems( const K3bCdDevice::DiskInfo& info )
{
  KListViewItem* atipItem = new HeaderViewItem( m_infoView, m_infoView->lastItem(), i18n("Media") );
  QString typeStr;
  if( info.currentProfile() != K3bCdDevice::MEDIA_UNKNOWN )
    typeStr = K3bCdDevice::mediaTypeString( info.currentProfile() );
  else if( info.mediaType() != K3bCdDevice::MEDIA_UNKNOWN )
    typeStr = K3bCdDevice::mediaTypeString( info.mediaType() );
  else
    typeStr = i18n("Unknown (probably CD-ROM)");

  KListViewItem* atipChild = new KListViewItem( atipItem, i18n("Type:"), typeStr );


  atipChild = new KListViewItem( atipItem, atipChild,
				 i18n("Size:"),
				 i18n("%1 min").arg(info.capacity().toString()),
				 KIO::convertSize(info.capacity().mode1Bytes()) );

  atipChild = new KListViewItem( atipItem, atipChild,
				 i18n("Remaining:"),
				 i18n("%1 min").arg( info.remainingSize().toString() ),
				 KIO::convertSize(info.remainingSize().mode1Bytes()) );

  atipChild = new KListViewItem( atipItem, atipChild,
				 i18n("Rewritable:"),
				 info.rewritable() ? i18n("yes") : i18n("no") );

  atipChild = new KListViewItem( atipItem, atipChild,
				 i18n("Appendable:"),
				 info.appendable() ? i18n("yes") : i18n("no") );

  atipChild = new KListViewItem( atipItem, atipChild,
				 i18n("Empty:"),
				 info.empty() ? i18n("yes") : i18n("no") );

  if( info.isDvdMedia() )
    atipChild = new KListViewItem( atipItem, atipChild,
				   i18n("Layers:"),
				   QString::number( info.numLayers() ) );
    
  if( info.mediaType() == K3bCdDevice::MEDIA_DVD_PLUS_RW ) {
    atipChild = new KListViewItem( atipItem, atipChild,
				   i18n("Background Format:") );
    switch( info.bgFormatState() ) {
    case K3bCdDevice::BG_FORMAT_NONE:
      atipChild->setText( 1, i18n("not formatted") );
      break;
    case K3bCdDevice::BG_FORMAT_INCOMPLETE:
      atipChild->setText( 1, i18n("incomplete") );
      break;
    case K3bCdDevice::BG_FORMAT_IN_PROGRESS:
      atipChild->setText( 1, i18n("in progress") );
      break;
    case K3bCdDevice::BG_FORMAT_COMPLETE:
      atipChild->setText( 1, i18n("complete") );
      break;
    }
  }
    
  atipChild = new KListViewItem( atipItem, atipChild,
				 i18n("Sessions:"),
				 QString::number( info.numSessions() ) );

  atipItem->setOpen( true );
}


void K3bDiskInfoView::createIso9660InfoItems( const K3bIso9660* iso )
{
  KListViewItem* iso9660Item = new HeaderViewItem( m_infoView, m_infoView->lastChild(), 
						   i18n("ISO9660 Filesystem Info") );
  KListViewItem* iso9660Child = 0;

  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("System Id:"),
				    iso->primaryDescriptor().systemId.isEmpty() 
				    ? QString("-")
				    : iso->primaryDescriptor().systemId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Volume Id:"),
				    iso->primaryDescriptor().volumeId.isEmpty() 
				    ? QString("-")
				    : iso->primaryDescriptor().volumeId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Volume Set Id:"),
				    iso->primaryDescriptor().volumeSetId.isEmpty()
				    ? QString("-")
				    : iso->primaryDescriptor().volumeSetId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Publisher Id:"),
				    iso->primaryDescriptor().publisherId.isEmpty()
				    ? QString("-") 
				    : iso->primaryDescriptor().publisherId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Preparer Id:"),
				    iso->primaryDescriptor().preparerId.isEmpty()
				    ? QString("-") 
				    : iso->primaryDescriptor().preparerId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Application Id:"),
				    iso->primaryDescriptor().applicationId.isEmpty()
				    ? QString("-") 
				    : iso->primaryDescriptor().applicationId );
//   iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
// 				    i18n("Volume Size:"),
// 				    QString( "%1 (%2*%3)" )
// 				    .arg(iso->primaryDescriptor().logicalBlockSize
// 					 *iso->primaryDescriptor().volumeSpaceSize)
// 				    .arg(iso->primaryDescriptor().logicalBlockSize)
// 				    .arg(iso->primaryDescriptor().volumeSpaceSize),
// 				    KIO::convertSize(iso->primaryDescriptor().logicalBlockSize
// 						     *iso->primaryDescriptor().volumeSpaceSize)  );

  iso9660Item->setOpen( true );
}


#include "k3bdiskinfoview.moc"

