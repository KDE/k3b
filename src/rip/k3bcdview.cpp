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
#include "../k3b.h"
#include "../tools/k3bglobals.h"
#include "k3bripperwidget.h"
#include "../k3bcddbmultientriesdialog.h"
#include "../kcutlabel.h"
#include "../k3btoolbox.h"

#include <qlayout.h>
#include <qmessagebox.h>
#include <qvaluelist.h>
#include <qfile.h>
#include <qlabel.h>
#include <qptrlist.h>

#include <kiconloader.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <kaction.h>
#include <klistview.h>
#include <kpopupmenu.h>
#include <kio/global.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kmessagebox.h>
#include <kdebug.h>




K3bCdView::K3bCdView( QWidget* parent, const char *name )
  : K3bCdContentsView( parent, name )
{
  m_cddb = new K3bCddb( this );

  connect( m_cddb, SIGNAL(infoMessage(const QString&)), 
	   k3bMain(), SLOT(showBusyInfo(const QString&)) );
  connect( m_cddb, SIGNAL(queryFinished(K3bCddb*)), 
	   k3bMain(), SLOT(endBusy()) );

  connect( m_cddb, SIGNAL(queryFinished(K3bCddb*)),
	   this, SLOT(slotCddbQueryFinished(K3bCddb*)) );

  setupActions();
  setupGUI();

  m_copyAction->setEnabled( !m_listView->selectedItems().isEmpty() );
}


K3bCdView::~K3bCdView()
{
}

void K3bCdView::setupGUI()
{
  QVBoxLayout* layout = new QVBoxLayout( this );
  layout->setAutoAdd( true );

  m_actionCollection = new KActionCollection( this );

  QHBox* cdInfoBox = new QHBox( this );
  cdInfoBox->setMargin( KDialog::marginHint() );
  cdInfoBox->setSpacing( KDialog::spacingHint() );

  //  (void)new QLabel( i18n("Artist: "), cdInfoBox );
  m_labelCdArtist = new KCutLabel( cdInfoBox );
//   (void)new QLabel( i18n("Title: "), cdInfoBox );
//   m_labelCdTitle = new QLabel( cdInfoBox );
//   (void)new QLabel( i18n("Extended Info: "), cdInfoBox );
//   m_labelCdExtInfo = new QLabel( cdInfoBox );


  m_listView = new KListView(this, "cdviewcontent");
  m_listView->addColumn(i18n( "No") );
  m_listView->addColumn(i18n( "Artist") );
  m_listView->addColumn(i18n( "Title") );
  m_listView->addColumn(i18n( "Time") );
  m_listView->addColumn(i18n( "Size") );
  m_listView->addColumn(i18n( "Type") );
  m_listView->addColumn(i18n( "Extended info") );
  m_listView->setItemsRenameable( false );
  m_listView->setShowSortIndicator(true);
  m_listView->setAllColumnsShowFocus(true);
  m_listView->setSelectionMode( QListView::Extended );

  connect( m_listView, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(slotContextMenu(KListView*, QListViewItem*, const QPoint&)) );
  connect( m_listView, SIGNAL(selectionChanged()),
	   this, SLOT(slotSelectionChanged()) );

  K3bToolBox* toolBox = new K3bToolBox( this );
  toolBox->addButton( m_copyAction );
}



void K3bCdView::setupActions()
{
  m_actionCollection = new KActionCollection( this );

  m_copyAction     = KStdAction::copy( this, SLOT(slotPrepareRipping()), m_actionCollection );
  m_selectAllAction = KStdAction::selectAll( this, SLOT(slotSelectAll()), m_actionCollection );
  m_deselectAllAction = KStdAction::deselect( this, SLOT(slotDeselectAll()), m_actionCollection );

  m_popupMenu = new KPopupMenu( this );
  m_selectAllAction->plug( m_popupMenu );
  m_deselectAllAction->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  m_copyAction->plug( m_popupMenu );
}


void K3bCdView::showCdView( const K3bDiskInfo& info )
{
  m_lastDiskInfo = info;

  m_listView->clear();

  int index = 1;
  for( K3bToc::const_iterator it = info.toc.begin(); it != info.toc.end(); ++it ) {

    // for now skip data tracks since we are not able to rip them to iso
    if( (*it).type() == K3bTrack::AUDIO )
      (void)new KListViewItem( m_listView, 
			       QString::number(index).rightJustify( 2, '0' ),
			       "", i18n("Track%1").arg(index),
			       K3b::framesToString( (*it).length(), false ),
			       KIO::convertSize( (*it).length() * 2352 ),
			       (*it).type() == K3bTrack::AUDIO ? i18n("Audio") : i18n("Data") );

    index++;
  }

  m_labelCdArtist->setText( i18n("Audio CD\n%1 minutes").arg( K3b::framesToString(m_lastDiskInfo.toc.length()) ) );

  m_lastQuery = K3bCddbQuery();

  KConfig* c = kapp->config();
  c->setGroup("Cddb");

  m_cddb->readConfig( c );

  if( c->readBoolEntry( "use local cddb query", true ) )
    m_cddb->localQuery( info.toc );
  else if( c->readBoolEntry( "use remote cddb", false ) ) {
    if( c->readBoolEntry( "query via cddbp", false ) )
      m_cddb->cddbpQuery( m_lastDiskInfo.toc );
    else
      m_cddb->httpQuery( m_lastDiskInfo.toc );
  }
}


void K3bCdView::slotCddbQueryFinished( K3bCddb* cddb )
{
  k3bMain()->endBusy();

  if( m_cddb->error() == K3bCddb::SUCCESS ) {
    m_lastQuery = cddb->queryResult();

    if( m_lastQuery.foundEntries() == 0 ) {
      KMessageBox::information( this, i18n("No CDDB entry found"), i18n("CDDB") );
    }
    else {
      m_lastSelectedCddbEntry = 0;
      if( m_lastQuery.foundEntries() > 1 ) {
	m_lastSelectedCddbEntry = K3bCddbMultiEntriesDialog::selectCddbEntry( m_lastQuery, this );
      }

      const K3bCddbEntry& entry = m_lastQuery.entry( m_lastSelectedCddbEntry );



      kdDebug() << "cddb info:" << endl;
      kdDebug() << "DTITLE:  '" << entry.cdTitle << "'" << endl;
      kdDebug() << "DARTIST: '" << entry.cdArtist << "'" << endl;
      kdDebug() << "DEXT:    '" << entry.cdExtInfo << "'" << endl;
      kdDebug() << "DISCID:  '" << entry.discid << "'" << endl;

      for( QStringList::const_iterator it = entry.titles.begin();
	   it != entry.titles.end(); ++it ) {
	kdDebug() << "TTITLE:  '" << *it << "'" << endl;
      }
      for( QStringList::const_iterator it = entry.artists.begin();
	   it != entry.artists.end(); ++it ) {
	kdDebug() << "TARTIST: '" << *it << "'" << endl;
      }
      for( QStringList::const_iterator it = entry.extInfos.begin();
	   it != entry.extInfos.end(); ++it ) {
	kdDebug() << "TEXT:    '" << *it << "'" << endl;
      }

      // now update the listview
      for( QListViewItemIterator it( m_listView ); it.current(); ++it ) {
	QListViewItem* item = it.current();
	int no = item->text(0).toInt() - 1;
	item->setText( 1, entry.artists[no] );
	item->setText( 2, entry.titles[no] );
	item->setText( 6, entry.extInfos[no] );
      }

      // and update the cd info 
      QString i = QString("%1 - %2\n").arg(entry.cdArtist).arg(entry.cdTitle);
      i.append( i18n("%1 minutes").arg( K3b::framesToString(m_lastDiskInfo.toc.length()) ) );
      if( !entry.cdExtInfo.isEmpty() )
	i.append( QString("\n(%1)").arg(entry.cdExtInfo) );
      m_labelCdArtist->setText( i );
//       m_labelCdTitle->setText( entry.cdTitle );
//       m_labelCdExtInfo->setText( entry.cdExtInfo );
    }
  }
  else {
    if( cddb->queryType() == K3bCddb::LOCAL ) {
      // do a remote query if configured
      KConfig* c = kapp->config();
      c->setGroup( "Cddb" );
      if( c->readBoolEntry( "use remote cddb", false ) ) {
	if( c->readBoolEntry( "query via cddbp", false ) )
	  m_cddb->cddbpQuery( m_lastDiskInfo.toc );
	else
	  m_cddb->httpQuery( m_lastDiskInfo.toc );
      }
      else {
	KMessageBox::information( this, i18n("No CDDB entry found"), i18n("CDDB") );
      }
    }
    else
      KMessageBox::information( this, i18n("No CDDB entry found"), i18n("CDDB") );
  }
}


void K3bCdView::slotPrepareRipping()
{
  QPtrList<QListViewItem> selectedList = m_listView->selectedItems();
  if( selectedList.isEmpty() ){
    QMessageBox::critical( this, i18n("Ripping Error"), i18n("Please select the title to rip."), i18n("OK") );
    return;
  }

  QValueList<int> trackNumbers;
  for( QPtrListIterator<QListViewItem> it( selectedList ); it.current(); ++it ) {
    trackNumbers.append( it.current()->text(0).toInt() );
  }

  K3bRipperWidget rip( m_lastDiskInfo, m_lastQuery.entry( m_lastSelectedCddbEntry ), trackNumbers, this );

  rip.exec();
}


void K3bCdView::slotSelectAll()
{
  m_listView->selectAll( true );
}

void K3bCdView::slotDeselectAll()
{
  m_listView->selectAll( false );
}


void K3bCdView::slotContextMenu( KListView*, QListViewItem*, const QPoint& p )
{
  m_popupMenu->exec(p);
}


void K3bCdView::slotSelectionChanged()
{
  m_copyAction->setEnabled( !m_listView->selectedItems().isEmpty() );
}



// ===========  slots ==============
void K3bCdView::reload()
{
}


#include "k3bcdview.moc"
