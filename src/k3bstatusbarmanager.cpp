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

#include "k3bstatusbarmanager.h"
#include "k3b.h"
#include "k3bbusywidget.h"

#include <kiconloader.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kio/global.h>
#include <kstatusbar.h>
#include <kaboutdata.h>

#include <qlabel.h>
#include <qhbox.h>
#include <qfile.h>
#include <qtimer.h>

#include <sys/vfs.h>

K3bStatusBarManager::K3bStatusBarManager( K3bMainWindow* parent )
  : QObject(parent),
    m_mainWindow(parent)
{
  // setup free temp space box
  QHBox* boxFreeTemp = new QHBox( m_mainWindow->statusBar() );
  boxFreeTemp->setSpacing(2);
  m_pixFreeTemp = new QLabel( boxFreeTemp );
  (void)new QLabel( i18n("Temp:"), boxFreeTemp );
  m_pixFreeTemp->setPixmap( SmallIcon("folder_green") );
  m_labelFreeTemp = new QLabel( boxFreeTemp );

  // busy widget
  m_busyWidget = new K3bBusyWidget( m_mainWindow->statusBar() );

  // setup info area
  m_labelInfoMessage = new QLabel( " ", m_mainWindow->statusBar() );

  // setup version info
  QLabel* versionBox = new QLabel( QString("K3b %1").arg(kapp->aboutData()->version()), m_mainWindow->statusBar() );

  // setup the statusbar
  m_mainWindow->statusBar()->addWidget( m_labelInfoMessage, 1 ); // for showing some info
  m_mainWindow->statusBar()->addWidget( boxFreeTemp, 0, true );
  m_mainWindow->statusBar()->addWidget( m_busyWidget, 0, true );
  m_mainWindow->statusBar()->addWidget( versionBox, 0, true );

  connect( m_mainWindow, SIGNAL(configChanged(KConfig*)), this, SLOT(update()) );

  update();
}


K3bStatusBarManager::~K3bStatusBarManager()
{
}


void K3bStatusBarManager::update()
{
  kapp->config()->setGroup( "General Options" );
  QString tempdir = kapp->config()->readEntry( "Temp Dir", KGlobal::dirs()->resourceDirs( "tmp" ).first() );

  struct statfs fs;
  if ( ::statfs(tempdir.latin1(),&fs) == 0 ) {
     unsigned int kBfak = fs.f_bsize/1024;
     slotFreeTempSpace(tempdir,fs.f_blocks*kBfak,0L,fs.f_bavail*kBfak);
  }   
  else {
     m_labelFreeTemp->setText("No info");
  }
}


void K3bStatusBarManager::slotFreeTempSpace(const QString&, 
					    unsigned long kbSize, 
					    unsigned long, 
					    unsigned long kbAvail)
{
  m_labelFreeTemp->setText(KIO::convertSizeFromKB(kbAvail)  + "/" + 
	                   KIO::convertSizeFromKB(kbSize)  );

  // if we have less than 640 MB that is not good
  if( kbAvail < 655360 )
    m_pixFreeTemp->setPixmap( SmallIcon("folder_red") );
  else
    m_pixFreeTemp->setPixmap( SmallIcon("folder_green") );

  // update the display every second
  QTimer::singleShot( 1000, this, SLOT(update()) );
}


void K3bStatusBarManager::showBusyInfo( const QString& str )
{
  m_labelInfoMessage->setText( str );
  m_busyWidget->showBusy( true );
}


void K3bStatusBarManager::endBusy()
{
  m_labelInfoMessage->setText( " " );
  m_busyWidget->showBusy( false );
}

#include "k3bstatusbarmanager.moc"
