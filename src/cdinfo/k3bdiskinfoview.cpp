/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bdiskinfoview.h"
#include "k3bdiskinfodetector.h"

#include <device/k3bdiskinfo.h>
#include <device/k3bcdtext.h>
#include <device/k3bdeviceglobals.h>
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
    : K3bCdContentsView( parent, name )
{
  QVBoxLayout* mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( 2 );
  mainLayout->setSpacing( 0 );

  // header
  // ----------------------------------------------------------------------------
  QFrame* headerFrame = K3bStdGuiItems::purpleFrame( this );
  QLabel* labelLeftPic = new QLabel( headerFrame );
  labelLeftPic->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_left.png" )) );
  m_labelTocType = new QLabel( headerFrame );
  m_labelTocType->setPaletteBackgroundColor( QColor(201, 208, 255) );
  m_labelDiskPix = new QLabel( headerFrame );

  QFont f(m_labelTocType->font() );
  f.setBold( true );
  f.setPointSize( f.pointSize() + 2 );
  m_labelTocType->setFont( f );

  QHBoxLayout* headerLayout = new QHBoxLayout( headerFrame );
  headerLayout->setMargin( 2 );
  headerLayout->setSpacing( 0 );
  headerLayout->addWidget( labelLeftPic );
  headerLayout->addWidget( m_labelTocType );
  headerLayout->addWidget( m_labelDiskPix );
  headerLayout->setStretchFactor( m_labelTocType, 1 );

  mainLayout->addWidget( headerFrame );

  m_infoView = new KListView( this );
  mainLayout->addWidget( m_infoView );

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
  const K3bCdDevice::DiskInfo& info = did->diskInfo();

  m_infoView->clear();

  if( !info.valid ) {
    m_labelTocType->setText( i18n("K3b was unable to retrieve disk information.") );
    m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_nodisk.png" )) );
  } 
  else if( info.noDisk ) {
    (void)new QListViewItem( m_infoView, i18n("No Disk") );
    m_labelTocType->setText( i18n("No disk in drive") );
    m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_nodisk.png" )) );
  }
  else {

    if( info.empty ) {
      m_labelTocType->setText( i18n("Disk is empty") );
      m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_empty.png" )) );
    } 
    else {
      switch( info.tocType ) {
      case K3bDiskInfo::AUDIO:
        m_labelTocType->setText( i18n("Audio CD") );
        m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_audio.png" )) );
        break;
      case K3bDiskInfo::DATA:
        m_labelTocType->setText( info.isVCD ? i18n("Video CD") : i18n("Data CD") );
        m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_data.png" )) );
        break;
      case K3bDiskInfo::MIXED:
        m_labelTocType->setText( info.isVCD ? i18n("Video CD") : i18n("Mixed mode CD") );
        m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_mixed.png" )) );
        break;
      case K3bDiskInfo::DVD:
        m_labelTocType->setText( info.isVideoDvd ? i18n("Video DVD") : i18n("DVD") );
        m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_dvd.png" )) );
        break;
      }
    }

    createMediaInfoItems( did->ngDiskInfo() );


    // iso9660 info
    // /////////////////////////////////////////////////////////////////////////////////////
    if( did->iso9660() ) {
      (void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item
      createIso9660InfoItems( did->iso9660() );
    }

    // tracks
    // /////////////////////////////////////////////////////////////////////////////////////
    if( m_infoView->childCount() )
      (void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

    KListViewItem* trackHeaderItem = new HeaderViewItem( m_infoView, m_infoView->lastChild(), i18n("Tracks") );
    if( info.toc.isEmpty() )
      (void)new KListViewItem( trackHeaderItem, i18n("Disk is Empty") );
    else {
      // create header item
      KListViewItem* item = new KListViewItem( trackHeaderItem,
					       i18n("Type"),
					       i18n("Attributes"),
					       i18n("First-Last Sector"),
					       i18n("Length") );

      int lastSession = 0;

      // if we have multible sessions we create a header item for every session
      KListViewItem* trackItem = 0;
      if( info.sessions > 1 && info.toc[0].session() > 0 ) {
	trackItem = new HeaderViewItem( trackHeaderItem, item, i18n("Session %1").arg(1) );
	lastSession = 1;
      }
      else
	trackItem = trackHeaderItem;

      // create items for the tracks
      K3bToc::const_iterator it;
      int index = 1;
      for( it = info.toc.begin(); it != info.toc.end(); ++it ) {
        const K3bTrack& track = *it;

	if( info.sessions > 1 && track.session() != lastSession ) {
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
			    : ( track.preEmphasis() ?  i18n("incremental") : i18n("uninterrupted") ) ) );
        item->setText( 2, QString("%1 - %2").arg(track.firstSector().lba()).arg(track.lastSector().lba()) );
        item->setText( 3, QString::number( track.length().lba() ) + " (" + track.length().toString() + ")" );
        ++index;
      }

      trackItem->setOpen(true);
    }
    trackHeaderItem->setOpen( true );


    // CD-TEXT
    // /////////////////////////////////////////////////////////////////////////////////////
    if( !did->cdText().isEmpty() ) {
      if( m_infoView->childCount() )
	(void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

      KListViewItem* cdTextHeaderItem = new HeaderViewItem( m_infoView, 
							    m_infoView->lastChild(), 
							    i18n("CD-TEXT (excerpt)") );

      // TODO: global CDtext

      // create header item
      KListViewItem* item = new KListViewItem( cdTextHeaderItem,
					       i18n("Performer"),
					       i18n("Title"),
					       i18n("Songwriter"),
					       i18n("Composer") );
      int index = 1;
      for( unsigned int i = 0; i < did->cdText().count(); ++i ) {
        item = new KListViewItem( cdTextHeaderItem, item );
	item->setText( 0, QString::number(index).rightJustify( 2, ' ' ) + " " +
		       did->cdText().trackCdText(i).performer() );
	item->setText( 1, did->cdText().trackCdText(i).title() );
	item->setText( 2, did->cdText().trackCdText(i).songwriter() );
	item->setText( 3, did->cdText().trackCdText(i).composer() );
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


void K3bDiskInfoView::createMediaInfoItems( const K3bCdDevice::NextGenerationDiskInfo& info )
{
  KListViewItem* atipItem = new HeaderViewItem( m_infoView, m_infoView->lastItem(), i18n("Media") );
  QString typeStr;
  if( info.currentProfile() != -1 )
    typeStr = K3bCdDevice::mediaTypeString( info.currentProfile() );
  else if( info.mediaType() == -1 )
    typeStr = i18n("Unknown (probably CD-ROM)");
  else
    typeStr = K3bCdDevice::mediaTypeString( info.mediaType() );

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

  iso9660Item->setOpen( true );
}

#include "k3bdiskinfoview.moc"

