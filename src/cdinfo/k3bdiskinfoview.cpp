
#include "k3bdiskinfoview.h"

#include "k3bdiskinfo.h"
#include "k3bdiskinfodetector.h"
#include "../tools/k3bglobals.h"
#include "../kcutlabel.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qfont.h>
#include <qcolor.h>
#include <qheader.h>
#include <qgroupbox.h>
#include <qstring.h>

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


  m_isoInfoWidget = new QWidget( this );
  QGridLayout* isoInfoLayout = new QGridLayout( m_isoInfoWidget );
  isoInfoLayout->setMargin( 0 );
  isoInfoLayout->setSpacing( KDialog::spacingHint() );

  // we use KCutLabels here since some of the values go up to 128 characters
  // disabled because K3b hangs when text is set (probably some endless loop with resizing)
//   m_labelIsoId            = new KCutLabel( m_isoInfoWidget );
//   m_labelIsoSystemId      = new KCutLabel( m_isoInfoWidget );
//   m_labelIsoVolumeId      = new KCutLabel( m_isoInfoWidget );
//   m_labelIsoVolumeSetId   = new KCutLabel( m_isoInfoWidget );
//   m_labelIsoPublisherId   = new KCutLabel( m_isoInfoWidget );
//   m_labelIsoPreparerId    = new KCutLabel( m_isoInfoWidget );
//   m_labelIsoApplicationId = new KCutLabel( m_isoInfoWidget );

  m_labelIsoId            = new QLabel( m_isoInfoWidget );
  m_labelIsoSystemId      = new QLabel( m_isoInfoWidget );
  m_labelIsoVolumeId      = new QLabel( m_isoInfoWidget );
  m_labelIsoVolumeSetId   = new QLabel( m_isoInfoWidget );
  m_labelIsoPublisherId   = new QLabel( m_isoInfoWidget );
  m_labelIsoPreparerId    = new QLabel( m_isoInfoWidget );
  m_labelIsoApplicationId = new QLabel( m_isoInfoWidget );


  isoInfoLayout->addWidget( new QLabel( i18n("Id:"), m_isoInfoWidget ), 0, 0 );
  isoInfoLayout->addWidget( m_labelIsoId, 0, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("System Id:"), m_isoInfoWidget ), 1, 0 );
  isoInfoLayout->addWidget( m_labelIsoSystemId, 1, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("Volume Id:"), m_isoInfoWidget ), 2, 0 );
  isoInfoLayout->addWidget( m_labelIsoVolumeId, 2, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("Volume Set Id:"), m_isoInfoWidget ), 3, 0 );
  isoInfoLayout->addWidget( m_labelIsoVolumeSetId, 3, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("Publisher Id:"), m_isoInfoWidget ), 0, 3 );
  isoInfoLayout->addWidget( m_labelIsoPublisherId, 0, 4 );
  isoInfoLayout->addWidget( new QLabel( i18n("Preparer Id:"), m_isoInfoWidget ), 1, 3 );
  isoInfoLayout->addWidget( m_labelIsoPreparerId, 1, 4 );
  isoInfoLayout->addWidget( new QLabel( i18n("Application Id:"), m_isoInfoWidget ), 2, 3 );
  isoInfoLayout->addWidget( m_labelIsoApplicationId, 2, 4 );

  isoInfoLayout->addColSpacing( 2, 10 );
  isoInfoLayout->setColStretch( 5, 1 );

  f = m_labelSize->font();
  f.setBold( true );
  m_labelSize->setFont( f );
  m_labelRemaining->setFont( f );
  m_labelMediumManufactor->setFont( f );
  m_labelMediumType->setFont( f );
  m_labelCdrw->setFont( f );
  m_labelAppendable->setFont( f );
  m_labelSessions->setFont( f );
  m_labelIsoId->setFont( f );
  m_labelIsoSystemId->setFont( f );
  m_labelIsoVolumeId->setFont( f );
  m_labelIsoVolumeSetId->setFont( f );
  m_labelIsoPublisherId->setFont( f );
  m_labelIsoPreparerId->setFont( f );
  m_labelIsoApplicationId->setFont( f );

  m_trackView = new KListView( this );
  m_trackView->addColumn( i18n("Tracks") );
  m_trackView->addColumn( i18n("First Sector") );
  m_trackView->addColumn( i18n("Last Sector") );
  m_trackView->addColumn( i18n("Length") );
  m_trackView->setFullWidth();
  m_trackView->setRootIsDecorated( true );
  m_trackView->setSorting(-1);
  m_trackView->header()->setClickEnabled( false );


  QFrame* line = new QFrame( this );
  line->setMargin( 0 );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  m_line = new QFrame( this );
  m_line->setMargin( 0 );
  m_line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  mainLayout->addWidget( m_labelDiskPix, 0, 0 );
  mainLayout->addWidget( m_labelTocType, 0, 1 );
  mainLayout->addMultiCellWidget( line, 1, 1, 0, 1 );
  mainLayout->addMultiCellWidget( m_infoWidget, 2, 2, 0, 1 );
  mainLayout->addMultiCellWidget( m_line, 3, 3, 0, 1 );
  mainLayout->addMultiCellWidget( m_isoInfoWidget, 4, 4, 0, 1 );
  mainLayout->addRowSpacing( 5, 10 );
  mainLayout->addMultiCellWidget( m_trackView, 6, 6, 0, 1 );
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
    m_isoInfoWidget->hide();
    m_line->hide();
    (void)new QListViewItem( m_trackView, i18n("No disk") );
    m_labelTocType->setText( i18n("No disk in drive") );
    m_labelDiskPix->setPixmap( SmallIcon( "stop" ) );
  }


  else {
    bool showLine = false;

    // check if we have some atip info
    if( info.size > 0 || !info.mediumManufactor.isEmpty() || info.sessions > 0 ) {
      m_infoWidget->show();
      showLine = true;

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

    if( !info.empty && 
	( info.tocType == K3bDiskInfo::DATA || info.tocType == K3bDiskInfo::DVD ) ) {
      m_isoInfoWidget->show();
      if( showLine ) showLine = true;

      m_labelIsoId->setText( info.isoId.isEmpty() ? QString("-") : info.isoId );
      m_labelIsoSystemId->setText( info.isoSystemId.isEmpty() ? QString("-") : info.isoSystemId );
      m_labelIsoVolumeId->setText( info.isoVolumeId.isEmpty() ? QString("-") : info.isoVolumeId );
      m_labelIsoVolumeSetId->setText( info.isoVolumeSetId.isEmpty() ? QString("-") : info.isoVolumeSetId );
      m_labelIsoPublisherId->setText( info.isoPublisherId.isEmpty() ? QString("-") : info.isoPublisherId );
      m_labelIsoPreparerId->setText( info.isoPreparerId.isEmpty() ? QString("-") : info.isoPreparerId );
      m_labelIsoApplicationId->setText( info.isoApplicationId.isEmpty() ? QString("-") : info.isoApplicationId );
    }
    else {
      m_isoInfoWidget->hide();
      showLine = false;
    }
       
    if( showLine ) {
      m_line->show();
    }
    else
      m_line->hide();
    

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

