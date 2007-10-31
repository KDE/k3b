/*
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include <config.h>


#include "k3bdiskinfoview.h"

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


// FIXME: use K3bListViewItem instead
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
  : K3bMediaContentsView( true,
			  K3bMedium::CONTENT_ALL,
			  K3bDevice::MEDIA_ALL|K3bDevice::MEDIA_UNKNOWN,
			  K3bDevice::STATE_EMPTY|K3bDevice::STATE_INCOMPLETE|K3bDevice::STATE_COMPLETE|K3bDevice::STATE_UNKNOWN,
			  parent, name )
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
#ifdef K3B_DEBUG
  m_infoView->addColumn( "index0" );
#endif

  m_infoView->header()->hide();

  // do not automatically reload the disk info
  //  setAutoReload( false );
}


K3bDiskInfoView::~K3bDiskInfoView()
{}


void K3bDiskInfoView::reloadMedium()
{
  m_infoView->clear();

  setTitle( medium().shortString( true ) );

  if( medium().diskInfo().diskState() == K3bDevice::STATE_NO_MEDIA ) {
    (void)new QListViewItem( m_infoView, i18n("No medium present") );
    setRightPixmap( K3bTheme::MEDIA_NONE );
  }
  else {
    if( medium().diskInfo().empty() ) {
      setRightPixmap( K3bTheme::MEDIA_EMPTY );
    }
    else {
      switch( medium().toc().contentType() ) {
      case K3bDevice::AUDIO:
	setRightPixmap( K3bTheme::MEDIA_AUDIO );
        break;
      case K3bDevice::DATA: {
	if( medium().content() & K3bMedium::CONTENT_VIDEO_DVD ) {
	  setRightPixmap( K3bTheme::MEDIA_VIDEO );
	}
	else {
	  setRightPixmap( K3bTheme::MEDIA_DATA );
	}
        break;
      }
      case K3bDevice::MIXED:
	setRightPixmap( K3bTheme::MEDIA_MIXED );
        break;
      default:
	setTitle( i18n("Unknown Disk Type") );
	setRightPixmap( K3bTheme::MEDIA_NONE );
      }
    }

    createMediaInfoItems( medium() );


    // iso9660 info
    // /////////////////////////////////////////////////////////////////////////////////////
    if( medium().content() & K3bMedium::CONTENT_DATA ) {
      (void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item
      createIso9660InfoItems( medium().iso9660Descriptor() );
    }

    // tracks
    // /////////////////////////////////////////////////////////////////////////////////////
    if( !medium().toc().isEmpty() ) {

      if( m_infoView->childCount() )
	(void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

      KListViewItem* trackHeaderItem = new HeaderViewItem( m_infoView, m_infoView->lastChild(), i18n("Tracks") );

      // create header item
      KListViewItem* item = new KListViewItem( trackHeaderItem,
					       i18n("Type"),
					       i18n("Attributes"),
					       i18n("First-Last Sector"),
					       i18n("Length") );

#ifdef K3B_DEBUG
      item->setText( 4, "Index0" );
#endif

      int lastSession = 0;

      // if we have multiple sessions we create a header item for every session
      KListViewItem* trackItem = 0;
      if( medium().diskInfo().numSessions() > 1 && medium().toc()[0].session() > 0 ) {
	trackItem = new HeaderViewItem( trackHeaderItem, item, i18n("Session %1").arg(1) );
	lastSession = 1;
      }
      else
	trackItem = trackHeaderItem;

      // create items for the tracks
      K3bDevice::Toc::const_iterator it;
      int index = 1;
      for( it = medium().toc().begin(); it != medium().toc().end(); ++it ) {
        const K3bTrack& track = *it;

	if( medium().diskInfo().numSessions() > 1 && track.session() != lastSession ) {
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

#ifdef K3B_DEBUG
        if( track.type() == K3bTrack::AUDIO )
	  item->setText( 4, QString( "%1 (%2)" ).arg(track.index0().toString()).arg(track.index0().lba()) );
#endif
        ++index;
      }

      trackItem->setOpen(true);
      trackHeaderItem->setOpen( true );
    }


    // CD-TEXT
    // /////////////////////////////////////////////////////////////////////////////////////
    if( !medium().cdText().isEmpty() ) {
      medium().cdText().debug();
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
		     medium().cdText().performer() );
      item->setText( 1, medium().cdText().title() );
      item->setText( 2, medium().cdText().songwriter() );
      item->setText( 3, medium().cdText().composer() );

      int index = 1;
      for( unsigned int i = 0; i < medium().cdText().count(); ++i ) {
        item = new KListViewItem( cdTextHeaderItem, item );
	item->setText( 0, QString::number(index).rightJustify( 2, ' ' ) + " " +
		       medium().cdText().at(i).performer() );
	item->setText( 1, medium().cdText().at(i).title() );
	item->setText( 2, medium().cdText().at(i).songwriter() );
	item->setText( 3, medium().cdText().at(i).composer() );
	++index;
      }

      cdTextHeaderItem->setOpen( true );
    }
  }
}


void K3bDiskInfoView::createMediaInfoItems( const K3bMedium& medium )
{
  const K3bDevice::DiskInfo& info = medium.diskInfo();

  KListViewItem* atipItem = new HeaderViewItem( m_infoView, m_infoView->lastItem(), i18n("Medium") );
  QString typeStr;
  if( info.mediaType() != K3bDevice::MEDIA_UNKNOWN )
    typeStr = K3bDevice::mediaTypeString( info.mediaType() );
  else
    typeStr = i18n("Unknown (probably CD-ROM)");

  KListViewItem* atipChild = new KListViewItem( atipItem, i18n("Type:"), typeStr );

  if( info.isDvdMedia() )
    atipChild = new KListViewItem( atipItem, atipChild,
				   i18n("Media ID:"),
				   !info.mediaId().isEmpty() ? QString::fromLatin1( info.mediaId() ) : i18n("unknown") );


  atipChild = new KListViewItem( atipItem, atipChild,
				 i18n("Capacity:"),
				 i18n("%1 min").arg(info.capacity().toString()),
				 KIO::convertSize(info.capacity().mode1Bytes()) );

  if( !info.empty() )
    atipChild = new KListViewItem( atipItem, atipChild,
				   i18n("Used Capacity:"),
				   i18n("%1 min").arg(info.size().toString()),
				   KIO::convertSize(info.size().mode1Bytes()) );

  if( info.appendable() )
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

  if( K3bDevice::isDvdMedia( info.mediaType() ) )
    atipChild = new KListViewItem( atipItem, atipChild,
				   i18n("Layers:"),
				   QString::number( info.numLayers() ) );

  if( info.mediaType() == K3bDevice::MEDIA_DVD_PLUS_RW ) {
    atipChild = new KListViewItem( atipItem, atipChild,
				   i18n("Background Format:") );
    switch( info.bgFormatState() ) {
    case K3bDevice::BG_FORMAT_NONE:
      atipChild->setText( 1, i18n("not formatted") );
      break;
    case K3bDevice::BG_FORMAT_INCOMPLETE:
      atipChild->setText( 1, i18n("incomplete") );
      break;
    case K3bDevice::BG_FORMAT_IN_PROGRESS:
      atipChild->setText( 1, i18n("in progress") );
      break;
    case K3bDevice::BG_FORMAT_COMPLETE:
      atipChild->setText( 1, i18n("complete") );
      break;
    }
  }

  atipChild = new KListViewItem( atipItem, atipChild,
				 i18n("Sessions:"),
				 QString::number( info.numSessions() ) );

  if( info.mediaType() & K3bDevice::MEDIA_WRITABLE ) {
    atipChild = new KListViewItem( atipItem, atipChild,
				   i18n("Supported writing speeds:") );
    QString s;
    if( medium.writingSpeeds().isEmpty() )
      s = "-";
    else
      for( QValueList<int>::const_iterator it = medium.writingSpeeds().begin();
	   it != medium.writingSpeeds().end(); ++it ) {
	if( !s.isEmpty() ) {
	  s.append( "\n" );
	  atipChild->setMultiLinesEnabled( true );
	}

	if( K3bDevice::isCdMedia( info.mediaType() ) )
	  s.append( QString( "%1x (%2 KB/s)" ).arg( *it/K3bDevice::SPEED_FACTOR_CD ).arg( *it ) );
	else if( K3bDevice::isDvdMedia( info.mediaType() ) )
	  s.append( QString().sprintf( "%.1fx (%d KB/s)", (double)*it / ( double )K3bDevice::SPEED_FACTOR_DVD, *it ) );
        else if ( K3bDevice::isBdMedia( info.mediaType() ) )
            s.append( QString().sprintf( "%.1fx (%d KB/s)", (double)*it / ( double )K3bDevice::SPEED_FACTOR_BD, *it ) );
      }

    atipChild->setText( 1, s );
  }

  atipItem->setOpen( true );
}


void K3bDiskInfoView::createIso9660InfoItems( const K3bIso9660SimplePrimaryDescriptor& iso )
{
  KListViewItem* iso9660Item = new HeaderViewItem( m_infoView, m_infoView->lastChild(),
						   i18n("ISO9660 Filesystem Info") );
  KListViewItem* iso9660Child = 0;

  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("System Id:"),
				    iso.systemId.isEmpty()
				    ? QString("-")
				    : iso.systemId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Volume Id:"),
				    iso.volumeId.isEmpty()
				    ? QString("-")
				    : iso.volumeId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Volume Set Id:"),
				    iso.volumeSetId.isEmpty()
				    ? QString("-")
				    : iso.volumeSetId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Publisher Id:"),
				    iso.publisherId.isEmpty()
				    ? QString("-")
				    : iso.publisherId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Preparer Id:"),
				    iso.preparerId.isEmpty()
				    ? QString("-")
				    : iso.preparerId );
  iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
				    i18n("Application Id:"),
				    iso.applicationId.isEmpty()
				    ? QString("-")
				    : iso.applicationId );
//   iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
// 				    i18n("Volume Size:"),
// 				    QString( "%1 (%2*%3)" )
// 				    .arg(iso.logicalBlockSize
// 					 *iso.volumeSpaceSize)
// 				    .arg(iso.logicalBlockSize)
// 				    .arg(iso.volumeSpaceSize),
// 				    KIO::convertSize(iso.logicalBlockSize
// 						     *iso.volumeSpaceSize)  );

  iso9660Item->setOpen( true );
}


void K3bDiskInfoView::enableInteraction( bool enable )
{
  QListViewItemIterator it( m_infoView );
  while( it.current() ) {
    it.current()->setEnabled( enable );
    ++it;
  }
}

#include "k3bdiskinfoview.moc"

