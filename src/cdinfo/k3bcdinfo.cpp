/***************************************************************************
                          k3bcdinfo.cpp  -  description
                             -------------------
    begin                : Sun Apr 22 2001
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

#include "k3bcdinfo.h"

#include "../device/k3bdevice.h"
#include "../device/k3btoc.h"
#include "../device/k3btrack.h"
#include "../k3bcddb.h"

#include <qlayout.h>
#include <qheader.h>
#include <qtimer.h>

#include <kprocess.h>
#include <kaction.h>
#include <klocale.h>
#include <kdialog.h>
#include <kapp.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klistview.h>
#include <kconfig.h>



K3bCdInfo::K3bCdInfo( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  m_device = 0;
  m_cdinfo = new PrivateCDInfo;

  m_infoTimer = new QTimer( this );
  connect( m_infoTimer, SIGNAL(timeout()), this, SLOT(slotTestDevice()) );

  m_process = new KProcess();
  m_actionRefresh = new KAction( i18n("Refresh"), "reload", 0, this, SLOT(slotRefresh()), this, "m_actionRefresh" );

  m_viewDiskInfo = new KListView( this );
  m_viewToc = new KListView( this );
  QVBoxLayout* box = new QVBoxLayout( this );
  box->setSpacing( KDialog::spacingHint() );
  box->setMargin( KDialog::marginHint() );

  box->addWidget( m_viewDiskInfo );
  box->addWidget( m_viewToc );

  m_viewDiskInfo->addColumn( "name" );
  m_viewDiskInfo->addColumn( "info" );
  m_viewDiskInfo->header()->hide();
  m_viewDiskInfo->setRootIsDecorated( true );
  m_viewDiskInfo->setSorting( -1 );

  m_viewToc->addColumn( i18n("title") );
  m_viewToc->addColumn( i18n("type") );
  m_viewToc->addColumn( i18n("first sector") );
  m_viewToc->addColumn( i18n("length") );
  m_viewToc->addColumn( i18n("last sector") );
  m_viewToc->setRootIsDecorated( true );
  m_viewToc->setSorting( -1 );

  clear();
}


K3bCdInfo::~K3bCdInfo()
{
  delete m_process;
}


void K3bCdInfo::clear()
{
  m_viewDiskInfo->clear();
  m_viewToc->clear();
  m_viewDiskInfo->hide();
  (void)new QListViewItem( m_viewToc, i18n("No medium inserted") );
}

void K3bCdInfo::setDevice( K3bDevice* dev )
{
  m_device = dev;
  slotRefresh();
}


void K3bCdInfo::slotTestDevice()
{
  tries++;

  if( cdrom_drive* drive = m_device->open() )
    {
      m_infoTimer->stop();

      qDebug( "(K3bCdInfo) Retrieving toc info..." );

      m_cdinfo->toc = m_device->readToc();
      if( !m_cdinfo->toc.isEmpty() ) {
	m_cdinfo->tocInfo_valid = true;

	KConfig* c = kapp->config();
	c->setGroup( "Cddb" );

	if( c->readBoolEntry( "useCddb", false ) )
	  if( K3bCddb::appendCddbInfo( m_cdinfo->toc ) ) {
	    m_cdinfo->cddbInfo_valid = true;
	  }
      }

      qDebug( "(K3bCdInfo) Finished." );

      // only cd-writers are able to retrieve atip-info
      if( m_device->burner() ) {
	// start cdrdao process
	m_process->clearArguments();
	m_process->disconnect();
	
	kapp->config()->setGroup("External Programs");
	*m_process << kapp->config()->readEntry( "cdrdao path" );
	*m_process << "disk-info";
	*m_process << "--device" << m_device->genericDevice();
	
	connect( m_process, SIGNAL(processExited(KProcess*)),
		 this, SLOT(slotCdrdaoFinished()) );
	connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
		 this, SLOT(slotParseCdrdaoOutput(KProcess*, char*, int)) );
	connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
		 this, SLOT(slotParseCdrdaoOutput(KProcess*, char*, int)) );
	
	if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
	  {
	    qDebug( "(K3bCdInfo) Could not start cdrdao!" );
	    m_process->disconnect();
	    
	    updateView();
	  }
	else
	  m_cdinfo->show_atip = true;
      }
      else
	updateView();
    }
  
  else if( tries > 10 ) {
    m_infoTimer->stop();
    clear();
  } 
}


void K3bCdInfo::slotRefresh()
{
  if( m_device == 0 )
    return;

  if( m_process->isRunning() )
    m_process->kill();

  clear();
  m_cdinfo->reset();
  tries = 0;

  m_viewToc->clear();
  (void)new QListViewItem( m_viewToc, i18n("Retrieving info...") );

  // try to get information every second for 10 seconds
  m_infoTimer->start(1000);
}


void K3bCdInfo::slotParseCdrdaoOutput( KProcess*, char* data, int len )
{
  QString buffer = QString::fromLatin1( data, len );
	
  // split to lines
  QStringList lines = QStringList::split( "\n", buffer );
	
  // do every line
  for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
    {
      *str = (*str).stripWhiteSpace();
      QString& line = *str;
      if( line.startsWith("CD-RW") )
	{
	  m_cdinfo->cdrw = line.right(3).contains("yes");
	  m_cdinfo->cdrw_valid = true;
	}
      else if( line.startsWith("Total") )
	{
	  m_cdinfo->size = line.mid( line.find(':') + 2 );
	  m_cdinfo->size_valid = true;
	}
      else if( line.startsWith("Remaining") )
	{
	  m_cdinfo->remaining = line.mid( line.find(':') + 2 );
	  m_cdinfo->remaining_valid = true;
	}
      else if( line.startsWith("CD-R medium") )
	{
	  m_cdinfo->medium = line.mid( line.find(':') + 2 );
	  m_cdinfo->medium_valid = true;
	}
      else if( line.startsWith("CD-R empty") )
	{
	  m_cdinfo->empty = line.right(3).contains("yes");
	  m_cdinfo->empty_valid = true;
	}
      else if( line.startsWith("Toc Type") )
	{
	  m_cdinfo->tocType = line.mid( line.find(':') + 2 );
	  m_cdinfo->tocType_valid = true;
	}
      else if( line.startsWith("Appendable") )
	{
	  m_cdinfo->appendable = line.right(3).contains("yes");
	  m_cdinfo->appendable_valid = true;
	}
      else if( line.startsWith("Sessions") )
	{
	  m_cdinfo->sessions = line.mid( line.find(':') + 2 ).toInt();
	  m_cdinfo->sessions_valid = true;
	}

      else if( line.startsWith("ERROR") )
	{
	  // some error occured
	  qDebug( line );
	}
    }
}


void K3bCdInfo::slotParseCdrecordOutput( KProcess*, char* data, int len )
{
//   QString buffer = QString::fromLatin1( data, len );
	
//   // split to lines
//   QStringList lines = QStringList::split( "\n", buffer );
	
//   // do every line
//   for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
//     {
//       *str = (*str).stripWhiteSpace();
//       QString& line = *str;
//       if( line.contains("Cannot read TOC") )
// 	{
// 	  // since there should be a cd in the drive it must be empty
// 	  m_cdinfo->tocInfo_valid = false;
// 	}
//       else if( line.startsWith("track:") )
// 	{
// 	  // we found a track-info and append it to the list
// 	  m_cdinfo->tocInfo_valid = true;
// 	  PrivateTrackInfo* newTrack = new PrivateTrackInfo;
// 	  QStringList tokens = QStringList::split( ":", line );
// 	  bool ok;
// 	  tokens[1].truncate( tokens[1].find('l') );
// 	  newTrack->number = tokens[1].toInt(&ok);
// 	  if( !ok )
// 	    newTrack->leadout = true;
// 	  else
// 	    newTrack->leadout = false;

// 	  tokens[2].truncate( tokens[2].find('(') );
// 	  newTrack->startBlock = tokens[2].toInt();
// 	  tokens[6].truncate( tokens[6].find('m') );
// 	  newTrack->control = tokens[6].toInt();
// 	  newTrack->mode = tokens[7].toInt();

// 	  m_cdinfo->tracks.prepend(newTrack);
// 	}
//     }
}


void K3bCdInfo::slotCdrdaoFinished()
{
  updateView();
}


void K3bCdInfo::slotCdrecordFinished()
{
  updateView();
}


void K3bCdInfo::updateView()
{
  m_viewToc->clear();
  m_viewDiskInfo->clear();

  QListViewItem *root1, *root2;
  QListViewItem *item1, *item2;

  // create ATIP-info items
  // ----------------------------------------------------
  if( m_cdinfo->show_atip ) {
    root1 = new QListViewItem( m_viewDiskInfo, "Disk info" );
    root1->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdrom_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );

    if( m_cdinfo->cdrw_valid ) {
      item1 = new QListViewItem( root1, i18n("CD-RW") );
      if( m_cdinfo->cdrw )
	item1->setText( 1, i18n("yes") );
      else
	item1->setText( 1, i18n("no") );
    }

    if( m_cdinfo->size_valid )
      item2 = new QListViewItem( root1, item1, i18n("Total Capacity"), m_cdinfo->size );
    
    if( m_cdinfo->medium_valid )
      item1 = new QListViewItem( root1, item2, i18n("CD-R medium"), m_cdinfo->medium );
    
    if( m_cdinfo->empty_valid ) {
      item2 = new QListViewItem( root1, item1, i18n("CD-R empty") );
      if( m_cdinfo->empty )
	item2->setText( 1, i18n("yes") );
      else
	item2->setText( 1, i18n("no") );
    }
    
    if( m_cdinfo->tocType_valid )
      item1 = new QListViewItem( root1, item2, i18n("Toc Type"), m_cdinfo->tocType );
    
    if( m_cdinfo->sessions_valid )
      item2 = new QListViewItem( root1, item1, i18n("Sessions"), QString::number(m_cdinfo->sessions) );
    
    if( m_cdinfo->appendable_valid ) {
      item1 = new QListViewItem( root1, item2, i18n("Appendable") );
      if( m_cdinfo->appendable )
	item1->setText( 1, i18n("yes") );
      else
	item1->setText( 1, i18n("no") );
    }

    if( m_cdinfo->remaining_valid )
      item2 = new QListViewItem( root1, item1, i18n("Remaining Capacity"), m_cdinfo->remaining );
 
    root1->setOpen( true );

    m_viewDiskInfo->show();
  }
  // ------------------------------------------------------
  // -------------------------------------- ATIP-info items


  // create TOC-info items
  // ----------------------------------------------------
  if( m_cdinfo->tocInfo_valid ) {
    root1 = new QListViewItem( m_viewToc, i18n("Table of contents") );
    root1->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdrom_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );

    if( m_cdinfo->cddbInfo_valid )
      root2 = new QListViewItem( root1, m_cdinfo->toc.artist() + " - " + m_cdinfo->toc.album() );
    else
      root2 = root1;

    for( K3bTrack* track = m_cdinfo->toc.last(); track != 0; track = m_cdinfo->toc.prev() ) {
      (void)new QListViewItem( root2, 
			       track->title(),
			       ( track->type() == K3bTrack::AUDIO ? "Audio" : "Data" ),
			       QString::number( track->firstSector() ), 
			       QString::number( track->length() ),
			       QString::number( track->lastSector() ) );
			       
    }

    root1->setOpen( true );
    root2->setOpen( true );
  }
  else
    (void)new QListViewItem( m_viewToc, i18n("disc is empty") );
  // ------------------------------------------------------
  // --------------------------------------- TOC-info items
}
