
#include "k3bdiskinfoview.h"

#include "k3bdiskinfo.h"
#include "k3bdiskinfodetector.h"
#include "../tools/k3bglobals.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qfont.h>
#include <qcolor.h>
#include <qheader.h>
#include <qstring.h>

#include <kdialog.h>
#include <klocale.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kstandarddirs.h>



K3bDiskInfoView::K3bDiskInfoView( QWidget* parent, const char* name )
  : K3bCdContentsView( parent, name )
{
  QVBoxLayout* mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( 0 );
  mainLayout->setSpacing( 0 );

  QLabel* labelLeftPic = new QLabel( this );
  labelLeftPic->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_left.png" )) );
  m_labelTocType = new QLabel( this );
  m_labelTocType->setPaletteBackgroundColor( QColor(201, 208, 255) );
  m_labelDiskPix = new QLabel( this );

  QFont f(m_labelTocType->font() );
  f.setBold( true );
  f.setPointSize( f.pointSize() + 2 );
  m_labelTocType->setFont( f );

  QHBoxLayout* headerLayout = new QHBoxLayout;
  headerLayout->setMargin( 0 );
  headerLayout->setSpacing( 0 );
  headerLayout->addWidget( labelLeftPic );
  headerLayout->addWidget( m_labelTocType );
  headerLayout->addWidget( m_labelDiskPix );
  headerLayout->setStretchFactor( m_labelTocType, 1 );

  mainLayout->addLayout( headerLayout );

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
{
}


void K3bDiskInfoView::displayInfo( const K3bDiskInfo& info )
{
  m_infoView->clear();

  if( !info.valid ) {
    m_labelTocType->setText( i18n("Sorry, K3b was not able to retrieve disk information.") );
    m_labelDiskPix->setPixmap( QPixmap(locate( "data", "k3b/pics/diskinfo_nodisk.png" )) );
  }

  else if( info.noDisk ) {
    (void)new QListViewItem( m_infoView, i18n("No disk") );
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



    // check if we have some atip info
    if( info.size > 0 || !info.mediumManufactor.isEmpty() || info.sessions > 0 ) {

      KListViewItem* atipItem = new KListViewItem( m_infoView, i18n("Disk") );
      KListViewItem* atipChild = 0;
      
      if( info.size > 0 )
	atipChild = new KListViewItem( atipItem, atipChild,
				       i18n("Size:"),
				       i18n("%1 min").arg( K3b::framesToString(info.size) ),
				       i18n("%2 MB)").arg(info.size*2048/1024/1024) );

      if( info.remaining > 0 )
	atipChild = new KListViewItem( atipItem, atipChild,
				       i18n("Remaining:"), 
				       i18n("%1 min").arg( K3b::framesToString(info.remaining) ),
				       i18n("%2 MB)").arg(info.remaining*2048/1024/1024) );

      if( !info.mediumManufactor.isEmpty() ) {
	atipChild = new KListViewItem( atipItem, atipChild,
				       i18n("Medium:") );
	atipChild->setMultiLinesEnabled( true );
	atipChild->setText( 1, info.mediumManufactor + "\n" + info.mediumType );
      }

      atipChild = new KListViewItem( atipItem, atipChild,
				     i18n("CDRW:"),
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

      KListViewItem* iso9660Item = new KListViewItem( m_infoView, m_infoView->lastChild(), i18n("ISO9660 info") );
      KListViewItem* iso9660Child = 0;

      iso9660Child = new KListViewItem( iso9660Item, iso9660Child,
					i18n("Id:"),
					info.isoId.isEmpty() ? QString("-") : info.isoId );
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

      iso9660Item->setOpen( true );
    }


    // tracks
    // /////////////////////////////////////////////////////////////////////////////////////
    if( m_infoView->childCount() )
      (void)new KListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

    KListViewItem* trackItem = new KListViewItem( m_infoView, m_infoView->lastChild(), i18n("Tracks") );
    if( info.toc.isEmpty() )
      (void)new KListViewItem( trackItem, i18n("disk is empty") );
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
	}
	else {
	  item->setPixmap( 0, SmallIcon( "tar" ) );
	  if( track.mode() == K3bTrack::MODE1 )
	    text = i18n("Data/mode1");
	  else if( track.mode() == K3bTrack::MODE2 )
	    text = i18n("Data/mode2");
	  else
	    text = i18n("Data");
	}
	item->setText( 0, i18n("%1 (%2)").arg( QString::number(index).rightJustify( 2, '0' )).arg(text) );
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

