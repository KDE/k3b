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

#include <device/k3bdiskinfo.h>
#include <device/k3bdiskinfodetector.h>
#include <k3bglobals.h>
#include <k3bstdguiitems.h>

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


void K3bDiskInfoView::displayInfo( const K3bDiskInfo& info )
{
  m_infoView->clear();

  if( !info.valid ) {
    m_labelTocType->setText( i18n("K3b was unable to retrieve disk information.") );
    m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_nodisk.png" )) );
  } else if( info.noDisk ) {
    (void)new QListViewItem( m_infoView, i18n("No Disk") );
    m_labelTocType->setText( i18n("No disk in drive") );
    m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_nodisk.png" )) );
  } else {

    if( info.empty ) {
      m_labelTocType->setText( i18n("Disk is empty") );
      m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_empty.png" )) );
    } else {
      switch( info.tocType ) {
      case K3bDiskInfo::AUDIO:
        m_labelTocType->setText( i18n("Audio CD") );
        m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_audio.png" )) );
        break;
      case K3bDiskInfo::DATA:
        m_labelTocType->setText( i18n("Data CD") );
        m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_data.png" )) );
        break;
      case K3bDiskInfo::MIXED:
        m_labelTocType->setText( i18n("Mixed mode CD") );
        m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_mixed.png" )) );
        break;
      case K3bDiskInfo::DVD:
        m_labelTocType->setText( i18n("DVD") );
        m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_dvd.png" )) );
        break;
      }
    }


    // media type
    if( info.mediaType != K3bCdDevice::MEDIA_NONE ) {
      KListViewItem* mediaItem = new HeaderViewItem( m_infoView, m_infoView->lastChild(), i18n("Media") );
      QString typeStr;
      switch( info.mediaType ) {
      case K3bCdDevice::MEDIA_DVD_ROM:
	typeStr = "DVD-ROM";
	break;
      case K3bCdDevice::MEDIA_DVD_R:
	typeStr = "DVD-R";
	break;
      case K3bCdDevice::MEDIA_DVD_RW:
	typeStr = "DVD-RW";
	break;
      case K3bCdDevice::MEDIA_DVD_R_SEQ:
	typeStr = "DVD-R Sequential";
	break;
      case K3bCdDevice::MEDIA_DVD_RAM:
	typeStr = "DVD-RAM";
	break;
      case K3bCdDevice::MEDIA_DVD_RW_OVWR: 
	typeStr = "DVD-RW Restricted Overwrite";
	break;
      case K3bCdDevice::MEDIA_DVD_RW_SEQ:
	typeStr = "DVD-RW Sequential";
	break;
      case K3bCdDevice::MEDIA_DVD_PLUS_RW:
	typeStr = "DVD+RW";
	break;
      case K3bCdDevice::MEDIA_DVD_PLUS_R:
	typeStr = "DVD+R";
	break;
      case K3bCdDevice::MEDIA_CD_ROM:
	typeStr = "CD-ROM";
	break;
      case K3bCdDevice::MEDIA_CD_R:
	typeStr = "CD-R";
	break;
      case K3bCdDevice::MEDIA_CD_RW:
	typeStr = "CD-RW";
	break;
      default:
	typeStr = i18n("Unknown (probably CD-ROM)");
      }
      (void)new KListViewItem( mediaItem, i18n("Type:"), typeStr );
      mediaItem->setOpen( true );
    }

    // check if we have some atip info
    if( info.size > 0 || !info.mediumManufactor.isEmpty() ) {

      if( m_infoView->childCount() )
        (void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

      KListViewItem* atipItem = new HeaderViewItem( m_infoView, m_infoView->lastChild(), i18n("Disk") );
      KListViewItem* atipChild = 0;

      if( info.size > 0 )
        atipChild = new KListViewItem( atipItem, atipChild,
                                       i18n("Size:"),
                                       i18n("%1 min").arg(info.size.toString()),
                                       KIO::convertSize(info.size.mode1Bytes()) );

      if( info.remaining > 0 )
        atipChild = new KListViewItem( atipItem, atipChild,
                                       i18n("Remaining:"),
                                       i18n("%1 min").arg( info.remaining.toString() ),
                                       KIO::convertSize(info.remaining.mode1Bytes()) );

      if( !info.mediumManufactor.isEmpty() ) {
        atipChild = new TwoColumnViewItem( atipItem, atipChild,
                                           i18n("Medium:") );
        atipChild->setMultiLinesEnabled( true );
        atipChild->setText( 1, info.mediumManufactor + "\n" + info.mediumType );
      }

      atipChild = new KListViewItem( atipItem, atipChild,
                                     i18n("Rewritable:"),
                                     info.cdrw ? i18n("yes") : i18n("no") );

      atipChild = new KListViewItem( atipItem, atipChild,
                                     i18n("Appendable:"),
                                     info.appendable ? i18n("yes") : i18n("no") );

      atipChild = new KListViewItem( atipItem, atipChild,
                                     i18n("Sessions:"),
                                     QString::number( info.sessions ) );

      atipItem->setOpen( true );
    }


    // iso9660 info
    // /////////////////////////////////////////////////////////////////////////////////////
    if( !info.empty &&
        ( info.tocType == K3bDiskInfo::DATA || info.tocType == K3bDiskInfo::DVD ) ) {

      if( m_infoView->childCount() )
        (void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

      KListViewItem* iso9660Item = new HeaderViewItem( m_infoView, m_infoView->lastChild(), 
						       i18n("ISO9660 Filesystem Info") );
      KListViewItem* iso9660Child = 0;

//       iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
//                                         i18n("Id:"),
//                                         info.isoId.isEmpty() ? QString("-") : info.isoId );
      iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
                                        i18n("System Id:"),
                                        info.isoSystemId.isEmpty() ? QString("-") : info.isoSystemId );
      iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
                                        i18n("Volume Id:"),
                                        info.isoVolumeId.isEmpty() ? QString("-") : info.isoVolumeId );
      iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
                                        i18n("Volume Set Id:"),
                                        info.isoVolumeSetId.isEmpty() ? QString("-") : info.isoVolumeSetId );
      iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
                                        i18n("Publisher Id:"),
                                        info.isoPublisherId.isEmpty() ? QString("-") : info.isoPublisherId );
      iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
                                        i18n("Preparer Id:"),
                                        info.isoPreparerId.isEmpty() ? QString("-") : info.isoPreparerId );
      iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
                                        i18n("Application Id:"),
                                        info.isoApplicationId.isEmpty() ? QString("-") : info.isoApplicationId );
      iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
                                        i18n("Size:"),
                                        QString("%1").arg(info.isoSize) );

      iso9660Item->setOpen( true );
    }


    // tracks
    // /////////////////////////////////////////////////////////////////////////////////////
    if( m_infoView->childCount() )
      (void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

    KListViewItem* trackItem = new HeaderViewItem( m_infoView, m_infoView->lastChild(), i18n("Tracks") );
    if( info.toc.isEmpty() )
      (void)new KListViewItem( trackItem, i18n("Disk is Empty") );
    else {
      // create header item
      KListViewItem* item = new KListViewItem( trackItem,
                            i18n("Type"),
                            i18n("First Sector"),
                            i18n("Last Sector"),
                            i18n("Length") );

      // create items for the tracks
      K3bToc::const_iterator it;
      int index = 1;
      for( it = info.toc.begin(); it != info.toc.end(); ++it ) {
        const K3bTrack& track = *it;

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
        item->setText( 1, QString::number( track.firstSector() ) );
        item->setText( 2, QString::number( track.lastSector() ) );
        item->setText( 3, QString::number( track.length() ) );

        ++index;
      }
    }
    trackItem->setOpen( true );
  }
}



void K3bDiskInfoView::reload()
{
  //   if( m_currentInfo )
  //     m_diskInfoDetector->detect( m_currentInfo.device );
}


#include "k3bdiskinfoview.moc"

