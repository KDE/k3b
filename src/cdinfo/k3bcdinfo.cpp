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

#include <kprocess.h>
#include <kaction.h>
#include <klocale.h>
#include <qlayout.h>
#include <qheader.h>
#include <kdialog.h>
#include <qtimer.h>
#include <kapp.h>
#include <kconfig.h>



K3bCdInfo::K3bCdInfo( QWidget* parent, const char* name )
  : KListView( parent, name )
{
  m_device = 0;
  m_cdinfo = new PrivateCDInfo;

  m_infoTimer = new QTimer( this );
  connect( m_infoTimer, SIGNAL(timeout()), this, SLOT(slotTestDevice()) );

  m_process = new KProcess();
  m_actionRefresh = new KAction( i18n("Refresh"), "reload", 0, this, SLOT(slotRefresh()), this, "m_actionRefresh" );

  addColumn( "name" );
  addColumn( "info" );
  header()->hide();

  (void)new QListViewItem( this, i18n("No medium inserted") );
}


K3bCdInfo::~K3bCdInfo()
{
  delete m_process;
  delete m_cdinfo;
}


void K3bCdInfo::setDevice( K3bDevice* dev )
{
  m_device = dev;
  slotRefresh();
}


void K3bCdInfo::slotTestDevice()
{
  tries++;

  if( m_device->isReady() == 0 )
    {
      m_infoTimer->stop();
      // start cdrdao process
      m_process->clearArguments();
      kapp->config()->setGroup("External Programs");
      *m_process << kapp->config()->readEntry( "cdrdao path" );
      *m_process << "disk-info";
      *m_process << "--device" << m_device->devicename();

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
	  clear();
	  (void)new QListViewItem( this, i18n("Some error occured") );
	}
    }
  else if( tries > 10 ) {
    m_infoTimer->stop();
    clear();
    (void)new QListViewItem( this, i18n("No medium inserted") );
  } 
}


void K3bCdInfo::slotRefresh()
{
  if( m_device == 0 )
    return;

  m_cdinfo->reset();
  tries = 0;

  clear();
  (void)new QListViewItem( this, i18n("Retrieving info...") );

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
}


void K3bCdInfo::slotCdrdaoFinished()
{
  updateView();
}


void K3bCdInfo::slotCdrecordFinished()
{

}


void K3bCdInfo::updateView()
{
  clear();

  QListViewItem* item;

  if( m_cdinfo->cdrw_valid ) {
    item = new QListViewItem( this, i18n("CD-RW") );
    if( m_cdinfo->cdrw )
      item->setText( 1, i18n("yes") );
    else
      item->setText( 1, i18n("no") );
  }

  if( m_cdinfo->size_valid )
    item = new QListViewItem( this, i18n("Total Capacity"), m_cdinfo->size );

  if( m_cdinfo->medium_valid )
    item = new QListViewItem( this, i18n("CD-R medium"), m_cdinfo->medium );

  if( m_cdinfo->empty_valid ) {
    item = new QListViewItem( this, i18n("CD-R empty") );
    if( m_cdinfo->empty )
      item->setText( 1, i18n("yes") );
    else
      item->setText( 1, i18n("no") );
  }

  if( m_cdinfo->tocType_valid )
    item = new QListViewItem( this, i18n("Toc Type"), m_cdinfo->tocType );

  if( m_cdinfo->sessions_valid )
    item = new QListViewItem( this, i18n("Sessions"), QString::number(m_cdinfo->sessions) );

  if( m_cdinfo->appendable_valid ) {
    item = new QListViewItem( this, i18n("Appendable") );
    if( m_cdinfo->appendable )
      item->setText( 1, i18n("yes") );
    else
      item->setText( 1, i18n("no") );
  }
}
