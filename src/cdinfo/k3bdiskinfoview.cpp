
#include "k3bdiskinfoview.h"

#include "k3bdiskinfo.h"
#include "k3bdiskinfodetector.h"
#include "../k3bglobals.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qfont.h>
#include <qcolor.h>
#include <qheader.h>

#include <kdialog.h>
#include <klocale.h>
#include <klistview.h>
#include <kiconloader.h>


K3bDiskInfoView::K3bDiskInfoView( QWidget* parent, const char* name )
  : K3bCdContentsView( parent, name )
{
  QGridLayout* mainLayout = new QGridLayout( this );
  mainLayout->setMargin( KDialog::marginHint() );
  mainLayout->setSpacing( KDialog::spacingHint() );

  m_labelDiskPix = new QLabel( this );
  m_labelTocType = new QLabel( this );
  QFont f(m_labelTocType->font() );
  f.setBold( true );
  f.setPointSize( f.pointSize() + 2 );
  m_labelTocType->setFont( f );

  QFrame* line = new QFrame( this );
  line->setMargin( 0 );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  m_infoWidget            = new QWidget( this );

  QGridLayout* infoLayout = new QGridLayout( m_infoWidget );
  infoLayout->setSpacing( KDialog::spacingHint() );
  infoLayout->setMargin( 0 );

  m_labelSize             = new QLabel( m_infoWidget );
  m_labelRemaining        = new QLabel( m_infoWidget );
  m_labelMediumManufactor = new QLabel( m_infoWidget );
  m_labelMediumType       = new QLabel( m_infoWidget );
  m_labelCdrw             = new QLabel( m_infoWidget );
  m_labelAppendable       = new QLabel( m_infoWidget );
  m_labelSessions         = new QLabel( m_infoWidget );
  f = m_labelSize->font();
  f.setBold( true );
  m_labelSize->setFont( f );
  m_labelRemaining->setFont( f );
  m_labelMediumManufactor->setFont( f );
  m_labelMediumType->setFont( f );
  m_labelCdrw->setFont( f );
  m_labelAppendable->setFont( f );
  m_labelSessions->setFont( f );


  infoLayout->addWidget( new QLabel( i18n("Total Capacity of medium:"), m_infoWidget ), 0, 0 );
  infoLayout->addWidget( new QLabel( i18n("Remaining Capacity:"), m_infoWidget ), 1, 0 );
  infoLayout->addWidget( new QLabel( i18n("Medium type:"), m_infoWidget ), 2, 0 );
  infoLayout->addWidget( m_labelSize, 0, 1 );
  infoLayout->addWidget( m_labelRemaining, 1, 1 );
  infoLayout->addWidget( m_labelMediumManufactor, 2, 1 );
  infoLayout->addWidget( m_labelMediumType, 3, 1 );

  infoLayout->addWidget( new QLabel( i18n("Is rewritable:"), m_infoWidget ), 0, 3 );
  infoLayout->addWidget( new QLabel( i18n("Is appendable:"), m_infoWidget ), 1, 3 );
  infoLayout->addWidget( new QLabel( i18n("Number of Sessions:"), m_infoWidget ), 2, 3 );
  infoLayout->addWidget( m_labelCdrw, 0, 4 );
  infoLayout->addWidget( m_labelAppendable, 1, 4 );
  infoLayout->addWidget( m_labelSessions, 2, 4 );

  infoLayout->addColSpacing( 2, 10 );
  infoLayout->setColStretch( 5, 1 );

  m_trackView = new KListView( this );
  m_trackView->addColumn( i18n("Tracks") );
  m_trackView->addColumn( i18n("First Sector") );
  m_trackView->addColumn( i18n("Last Sector") );
  m_trackView->addColumn( i18n("Length") );
  m_trackView->setFullWidth();
  m_trackView->setRootIsDecorated( true );
  m_trackView->setSorting(-1);
  m_trackView->header()->setClickEnabled( false );


  mainLayout->addWidget( m_labelDiskPix, 0, 0 );
  mainLayout->addWidget( m_labelTocType, 0, 1 );
  mainLayout->addMultiCellWidget( line, 1, 1, 0, 1 );
  mainLayout->addMultiCellWidget( m_infoWidget, 2, 2, 0, 1 );
  mainLayout->addRowSpacing( 3, 10 );
  mainLayout->addMultiCellWidget( m_trackView, 4, 4, 0, 1 );
  mainLayout->setColStretch( 1, 1 );
}


K3bDiskInfoView::~K3bDiskInfoView()
{
}


void K3bDiskInfoView::displayInfo( const K3bDiskInfo& info )
{
  m_trackView->clear();

  // first check if there is a cd
  if( info.noDisk ) {
    m_infoWidget->hide();
    (void)new QListViewItem( m_trackView, i18n("No disk") );
    m_labelTocType->setText( i18n("No disk in drive") );
    m_labelDiskPix->setPixmap( SmallIcon( "stop" ) );
  }


  else {
    // check if we have some atip info
    if( info.sessions > 0 ) {
      m_infoWidget->show();

      if( info.size > 0 )
	m_labelSize->setText( QString("%1 (%2 MB)").arg( K3b::framesToString(info.size) ).arg(info.size*2048/1024/1024) );
      else
	m_labelSize->setText("-");

      if( info.remaining > 0 )
	m_labelRemaining->setText( QString("%1 (%2 MB)").arg( K3b::framesToString(info.remaining) ).arg(info.remaining*2048/1024/1024) );
      else
	m_labelRemaining->setText("-");

      if( !info.mediumManufactor.isEmpty() ) {
	m_labelMediumManufactor->setText( info.mediumManufactor );
	m_labelMediumType->setText( info.mediumType );
      }
      else {
	m_labelMediumManufactor->setText( "-" );
	m_labelMediumType->setText( QString::null );
      }

      m_labelCdrw->setText( info.cdrw ? i18n("yes") : i18n("no") );

      m_labelAppendable->setText( info.appendable ? i18n("yes") : i18n("no") );

      m_labelSessions->setText( QString::number( info.sessions ) );
    }
    else {
      m_infoWidget->hide();
    }


    if( info.empty ) {
      m_labelTocType->setText( i18n("Disk is empty") );
      m_labelDiskPix->setPixmap( SmallIcon( "cdrom_unmount", 32 ) );
    }
    else {
      switch( info.tocType ) {
      case K3bDiskInfo::AUDIO:
	m_labelTocType->setText( i18n("Audio CD") );
	m_labelDiskPix->setPixmap( SmallIcon( "cdaudio_unmount", 32 ) );
	break;
      case K3bDiskInfo::DATA:
	m_labelTocType->setText( i18n("Data CD") );
	m_labelDiskPix->setPixmap( SmallIcon( "cdrom_unmount", 32 ) );
	break;
      case K3bDiskInfo::MIXED:
	m_labelTocType->setText( i18n("Mixed mode CD") );
	m_labelDiskPix->setPixmap( SmallIcon( "cdrom_unmount", 32 ) );
	break;
      case K3bDiskInfo::DVD:
	m_labelTocType->setText( i18n("DVD") );
	m_labelDiskPix->setPixmap( SmallIcon( "dvd_unmount", 32 ) );
	break;
      }
    }


    if( info.toc.isEmpty() )
      (void)new QListViewItem( m_trackView, i18n("disk is empty") );
    else {
      // create items for the tracks
      K3bToc::const_iterator it;
      for( it = info.toc.begin(); it != info.toc.end(); ++it ) {
	const K3bTrack& track = *it;

	QListViewItem* item = new QListViewItem( m_trackView, m_trackView->lastItem() );
	if( track.type() == K3bTrack::AUDIO ) {
	  item->setPixmap( 0, SmallIcon( "sound" ) );
	  item->setText( 0, i18n("audio") );
	}
	else {
	  item->setPixmap( 0, SmallIcon( "tar" ) );
	  if( track.mode() == K3bTrack::MODE1 )
	    item->setText( 0, i18n("data/mode1") );
	  else if( track.mode() == K3bTrack::MODE2 )
	    item->setText( 0, i18n("data/mode2") );
	  else
	    item->setText( 0, i18n("data") );
	}

	item->setText( 1, QString::number( track.firstSector() ) );
	item->setText( 2, QString::number( track.lastSector() ) );
	item->setText( 3, QString::number( track.length() ) );
      }
    }
  }
}


void K3bDiskInfoView::reload()
{
//   if( m_currentInfo )
//     m_diskInfoDetector->detect( m_currentInfo.device );
}


#include "k3bdiskinfoview.moc"

