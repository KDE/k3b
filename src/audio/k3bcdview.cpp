/***************************************************************************
                          k3bcdview.cpp  -  description
                             -------------------
    begin                : Sun Oct 28 2001
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

#include "k3bcdview.h"
#include "k3bcdviewitem.h"
#include "k3bcddb.h"
#include "k3bcdda.h"
#include "../k3b.h"
#include "../k3boptiondialog.h"
#include "../k3bglobals.h"

#include <qwidget.h>
#include <qlayout.h>

#include <kaction.h>
#include <kiconloader.h>
#include <kicontheme.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>

#define RELOAD_BUTTON_INDEX    0
#define GRAB_BUTTON_INDEX        1
//#define DEFAULT_CDROM               "/dev/cdrom"
#define DEFAULT_CDDB_HOST       "localhost:888"

K3bCdView::K3bCdView(QWidget *parent, const char *name=0)
	: QWidget(parent, name){
	
  QGridLayout *_boxLayout = new QGridLayout( this, 2, 2, 0, -1, "cdviewlayout" );
  KToolBar *toolBar = new KToolBar( this, "cdviewtoolbar" );
  //K3bCdViewItem *_cdView = new K3bCdViewItem( this, "cditemview" );
   m_listView = new KListView(this, "cdviewcontent");
  	m_listView->addColumn(i18n( "No") );
	m_listView->addColumn(i18n( "Artist") );
	m_listView->addColumn(i18n( "Title") );
	m_listView->addColumn(i18n( "Time") );
	m_listView->addColumn(i18n( "Size") );
	m_listView->addColumn(i18n( "Filename") );
	m_listView->setItemsRenameable( false );

  KIconLoader *_il = new KIconLoader("k3b");
  toolBar->insertButton( _il->iconPath("reload", KIcon::Toolbar), 	RELOAD_BUTTON_INDEX);
  toolBar->insertButton( _il->iconPath("editcopy", KIcon::Toolbar), 	GRAB_BUTTON_INDEX);
  KToolBarButton *_buttonGrab = toolBar->getButton(GRAB_BUTTON_INDEX);
  KToolBarButton *_buttonReload = toolBar->getButton(RELOAD_BUTTON_INDEX);
  	
   _boxLayout->addWidget( toolBar, 0, 0 );
   _boxLayout->addMultiCellWidget( m_listView, 1, 1, 0, 2 );

  m_cddb = new K3bCddb(  );

  // connect to the actions
  connect( _buttonReload, SIGNAL(clicked()), this, SLOT(reload()) );
  connect( _buttonGrab, SIGNAL(clicked()), this, SLOT(grab()) );
}

K3bCdView::~K3bCdView(){
}

void K3bCdView::showCdContent(struct cdrom_drive *drive ){
	K3bCdda *_cdda = new K3bCdda();
	// read cddb settings each time to get changes in optiondialog
	applyOptions();
	// clear old entries
	m_listView->clear();
	m_cddb->updateCD( drive );
	QStringList titles = 	m_cddb->getTitles();
   // print it out
  int no = 1;
  // raw file length (wav has +44 byte header data)
  long totalByteCount = 0;
  QString filename;
  // TODO: add filename and length
  for ( QStringList::Iterator it = titles.begin(); it != titles.end(); ++it ) {
      totalByteCount = _cdda->getRawTrackSize(no, drive);
      filename = prepareFilename( (*it).latin1() );
      // add item to cdViewItem
      addItem(no, m_cddb->getArtist(), (*it).latin1(), K3b::sizeToTime(totalByteCount), totalByteCount, filename);
      no++;
  }
}

void K3bCdView::showCdView(QString device){
   m_device = device;
	K3bCdda *_cdda = new K3bCdda();
	m_drive = _cdda->pickDrive(m_device);
	showCdContent(m_drive);
	_cdda->closeDrive(m_drive);
}

// ===========  slots ==============
void K3bCdView::reload(){
	K3bCdda *_cdda = new K3bCdda();
	m_drive = _cdda->pickDrive(m_device);
	qDebug("(K3bCdView) Reload");
	showCdContent(m_drive);
	_cdda->closeDrive(m_drive);
}

void K3bCdView::grab(){
	qDebug("(K3bCdView) not implemented yet (alpha)");
	/*
	K3bCdda *_cdda = new K3bCdda();
	m_drive = _cdda->pickDrive(m_device);
   _cdda->paranoiaRead(m_drive, 1);
	_cdda->closeDrive(m_drive);
	*/
}
//  helpers
// -----------------------------------------
void K3bCdView::addItem(int no, QString artist, QString title, QString time, long length, QString filename){
	QString number;
   QString size;
   if (no < 10)
   	number = "0" + QString::number(no);
   else
   	number = QString::number(no);
	size = QString::number( (double) length / 1000000, 'g', 3) + " MB";
	KListViewItem *song = new KListViewItem(m_listView, number, artist, title, time, size, filename);
}

void K3bCdView::applyOptions(){
  KConfig *c = kapp->config();
  c->setGroup("Cddb");
  bool useCddb = c->readBoolEntry("useCddb", false);
  QString hostString = c->readEntry("cddbServer", DEFAULT_CDDB_HOST);
  int index = hostString.find(":");
  QString server = hostString.left(index);
  unsigned int port = hostString.right(hostString.length()-index-1).toUInt();
  qDebug("(K3bCdView) CddbServer: " + server + ":" + QString::number(port) );
  m_cddb->setServer(server);
  m_cddb->setPort(port);
  m_cddb->setUseCddb(useCddb);
}

QString K3bCdView::prepareFilename(QString filename){
   // parse filename and pattern it.
	return filename + ".wav";
}