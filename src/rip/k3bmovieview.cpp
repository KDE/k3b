/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bmovieview.h"
#include "k3bdvdcontent.h"
#include "k3btcwrapper.h"
#include "k3bdvdripperwidget.h"
#include <k3bdevice.h>
#include "k3bdvdriplistviewitem.h"
#include <kcutlabel.h>
#include <k3btoolbox.h>
#include <k3bstdguiitems.h>

#include <qstring.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qhgroupbox.h>
#include <qlabel.h>
//#include <qvaluelist.h>
//#include <qstringlist.h>
//#include <qlistview.h>
//#include <qmessagebox.h>
//#include <qpushbutton.h>
#include <qfont.h>

#include <kiconloader.h>
//#include <ktoolbar.h>
//#include <ktoolbarbutton.h>
//#include <kprocess.h>
#include <klocale.h>
#include <klistview.h>
#include <kdialog.h>
//#include <kapplication.h>
//#include <kconfig.h>
#include <kpopupmenu.h>
#include <kio/global.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <kdebug.h>

K3bMovieView::K3bMovieView(QWidget *parent, const char *name )
  : K3bCdContentsView( true, parent, name ) 
{
  m_tcWrapper = new K3bTcWrapper( this );
  connect( m_tcWrapper, SIGNAL( successfulDvdCheck( bool ) ), 
	   this, SLOT( slotDvdChecked( bool )) );
  setupGUI();
  setupActions();
}


K3bMovieView::~K3bMovieView()
{
  delete m_tcWrapper;
}

void K3bMovieView::setupGUI()
{
  setTitle( i18n("Video DVD") );
  setRightPixmap( "diskinfo_dvd" );

  QVBoxLayout* layout = new QVBoxLayout( mainWidget() );
  layout->setSpacing(0);

  // toolbox
  // ----------------------------------------------------------------------------------
  m_toolBox = new K3bToolBox( mainWidget() );
  m_labelDvdInfo = new KCutLabel( mainWidget() );
  m_labelDvdInfo->setMargin( 2 );

  m_listView = new KListView(mainWidget(), "cdviewcontent");
  m_listView->addColumn( i18n("Titles" ) );
  m_listView->addColumn( i18n("Time" ) );
  m_listView->addColumn( i18n("Language" ) );
  m_listView->addColumn( i18n("Chapter" ) );
  m_listView->addColumn( i18n("Angle" ) );
  //m_listView->setColumnWidthMode( 0, QListView::Manual );
  m_listView->setItemsRenameable( false );
  m_listView->setRootIsDecorated( true );
  m_listView->setAllColumnsShowFocus( true );
  //m_listView->setSorting( -1 );
  connect( m_listView, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(slotContextMenu(KListView*, QListViewItem*, const QPoint&)) );
  connect( m_listView, SIGNAL(selectionChanged( QListViewItem* )),
	   this, SLOT(slotTitleSelected( QListViewItem* )) );

  layout->addWidget( m_toolBox );
  layout->addWidget( m_listView );  
  layout->addWidget( m_labelDvdInfo );
}


void K3bMovieView::setupActions()
{
  m_actionCollection = new KActionCollection( this );
  KAction* copyAction = KStdAction::copy( this, SLOT(slotRip()), m_actionCollection );

  m_popupMenu = new KPopupMenu( this );
  copyAction->plug( m_popupMenu );
  m_toolBox->addButton( copyAction );
}


void K3bMovieView::setDevice( K3bDevice::Device* device )
{
  m_device = device;
}

void K3bMovieView::reload()
{
  // check if transcode tool installed
  if( !m_tcWrapper->supportDvd() ) {
    emit notSupportedDisc( m_device->devicename() );
    return;
  }
  KDialog* infoDialog = new KDialog( this, "waitForDiskInfoDialog", true, WType_Popup|WDestructiveClose );
  infoDialog->setCaption( i18n("Please Wait...") );
  QHBoxLayout* infoLayout = new QHBoxLayout( infoDialog );
  infoLayout->setSpacing( KDialog::spacingHint() );
  infoLayout->setMargin( KDialog::marginHint() );
  infoLayout->setAutoAdd( true );
  QLabel* picLabel = new QLabel( infoDialog );
  picLabel->setPixmap( DesktopIcon( "cdwriter_unmount" ) );
  (void)new QLabel( i18n("K3b is fetching information about title "), infoDialog );
  m_fetchingInfoLabel = new QLabel( "<1>", infoDialog );
  connect( m_tcWrapper, SIGNAL(tcprobeTitleParsed( int )), this, SLOT( slotUpdateInfoDialog( int )) );
  connect( m_tcWrapper, SIGNAL( successfulDvdCheck( bool )), infoDialog, SLOT( close() ));
  infoDialog->show();

  m_tcWrapper->checkDvdContent( m_device );
}

QString K3bMovieView::filterAudioList( QStringList *al )
{
  QString result("");
  for ( QStringList::Iterator it = al->begin(); it != al->end(); ++it ) {
    QStringList lang = QStringList::split(" ", (*it) );
    if( lang[1].contains("bit") || lang[1].contains("drc") ){
      lang[1] = "??";
    }
    if( !result.contains( lang[1] ) ){
      result += lang[1] +", "; // too much space for all info + " (" + lang[0] + "/" + lang.last() +"), ";
    }
    kdDebug() << "(K3bMovieView) Parse audio tracks for language: " << result << endl;
  }
  return result.left( result.length()-2);
}

// ------------------------------------------
// slots
// ------------------------------------------

void K3bMovieView::slotDvdChecked( bool successful )
{
  m_dvdTitles.clear();

  if( successful ){
    K3bDvdRipListViewItem *longestTitle = 0;
    long maxFrames = 0;
    m_dvdTitles = m_tcWrapper->getDvdTitles();
    m_listView->clear();
    QStringList existingTitlesets;
    for( unsigned int i= 0; i < m_dvdTitles.count();  i++){
      K3bDvdContent *title = &(m_dvdTitles[ i ]);
      int t=title->getTitleSet();
      QString strTitleset = QString::number( t );
      if ( t < 10 )
	strTitleset = "0" + strTitleset;
      QString mainEntryAndKey = i18n("Titleset %1").arg( strTitleset );
      kdDebug() << "(K3bMovieView) Titleset: " << QString::number( title->getTitleSet()) << " Title: " << QString::number( title->getTitleNumber() ) << endl;
      if( !existingTitlesets.contains( QString::number( title->getTitleSet()) ) ){
	existingTitlesets << QString::number( title->getTitleSet());
	K3bDvdRipListViewItem *item = new K3bDvdRipListViewItem( m_listView, mainEntryAndKey );
	item->setExpandable( true );
	item->setOpen( false );
	item->setHiddenTitle( -1 );
	item->setTitleSet( true );
      }
      K3bDvdRipListViewItem *titleItem =  new K3bDvdRipListViewItem( m_listView->findItem( mainEntryAndKey, 0), 
								     i18n("Title %1").arg(title->getTitleNumber() ),
								     title->getStrTime(), 
								     filterAudioList( title->getAudioList() ), 
								     QString::number( title->getMaxChapters() ), 
								     title->getAngles()->join(",") );
      titleItem->setHiddenTitle( title->getTitleNumber( ));
      if( title->getFrames() > maxFrames ){
	maxFrames = title->getFrames();
	longestTitle = titleItem;
      }
      /*
	if( existingTitlesets.contains( QString::number( title->getTitleSet()) ) ){
	K3bDvdRipListViewItem *titleItem =  new K3bDvdRipListViewItem( m_listView->findItem( mainEntryAndKey, 0), i18n("Title %1").arg(title->getTitleNumber() ),
	title->getStrTime(), filterAudioList( title->getAudioList() ), QString::number( title->getMaxChapters() ), title->getAngles()->join(",") );
	titleItem->setHiddenTitle( title->getTitleNumber( ));
	} else {
	existingTitlesets << QString::number( title->getTitleSet());
	K3bDvdRipListViewItem *item = new K3bDvdRipListViewItem( m_listView, mainEntryAndKey );
	item->setExpandable( true );
	item->setOpen( false );
	item->setHiddenTitle( -1 );
	item->setTitleSet( true );
	K3bDvdRipListViewItem *titleItem =  new K3bDvdRipListViewItem( m_listView->findItem( mainEntryAndKey, 0), i18n("Title %1").arg(title->getTitleNumber() ),
	title->getStrTime(), filterAudioList( title->getAudioList() ), QString::number( title->getMaxChapters() ), title->getAngles()->join(",") );
	titleItem->setHiddenTitle( title->getTitleNumber( ));
	}
      */
    }

    QWidget::show();
    if (longestTitle)
    {
    longestTitle->parent()->setOpen( true );
    m_listView->setSelected( longestTitle, true );
    }
  } else {
    // error during parsing
    emit notSupportedDisc( m_device->devicename() );
  }
}

void K3bMovieView::slotTitleSelected( QListViewItem *item)
{
  int title = (dynamic_cast<K3bDvdRipListViewItem*>(item) )->getHiddenTitle();
  if( title > 0 ){
    DvdTitle::Iterator dvd;
    dvd = m_dvdTitles.at( title-1 );
    QString ext = "";
    if( (*dvd).getStrAspectAnamorph().length() > 1 ){
      ext = "(" + (*dvd).getStrAspectAnamorph();
    }
    if( (*dvd).getStrAspectExtension().length() > 1 ){
      ( ext.length() > 1 ) ? ext += ", " : ext += "(";
      ext += (*dvd).getStrAspectExtension();
    }
    if( ext.length() > 1){
      ext = ext + ")";
    }
    m_labelDvdInfo->setText( i18n("Title %1 (Angle(s) %2)\nMoviedata - TV Norm: %3, Time: %4 hours, Frames: %5, FPS: %6\n\
Moviedata - Aspect Ratio %7 %8").arg(
				     (*dvd).getTitleNumber()).arg((*dvd).getAngles()->join(",")).arg((*dvd).getMode()).arg((*dvd).getStrTime()).arg((*dvd).getStrFrames()).arg(
																					       (*dvd).getStrFramerate()).arg((*dvd).getStrAspect()).arg(ext) );
  }
}

void K3bMovieView::slotNotSupportedDisc()
{
  emit notSupportedDisc( m_device->devicename() );
}

void K3bMovieView::slotUpdateInfoDialog( int i )
{
  m_fetchingInfoLabel->setText( "<"+QString::number( i )+">" );
}

void K3bMovieView::slotRip()
{
  K3bDvdRipperWidget ripWidget( m_device->devicename(), this, "dvdrip");
  DvdTitle::Iterator dvd;
  int title = m_ripTitle->getHiddenTitle( );
  kdDebug() << QString::number(title) << " Title" << endl;
  DvdTitle toRipTitles;
  // clear old angle selection if already used
  dvd = m_dvdTitles.at( title-1 );
  toRipTitles.append( *dvd );
  ripWidget.init( toRipTitles );
  ripWidget.exec();
}


void K3bMovieView::slotContextMenu( KListView*, QListViewItem *lvi, const QPoint& p )
{
  m_ripTitle = dynamic_cast<K3bDvdRipListViewItem*>(lvi);
  if( m_ripTitle)
	  m_popupMenu->exec(p);
}


#include "k3bmovieview.moc"

