/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiocdview.h"
#include "k3baudiorippingdialog.h"
#include "k3baudiocdlistview.h"

#include <k3btoc.h>
#include <k3bdiskinfo.h>
#include <k3bdevicehandler.h>
#include <k3blistview.h>
#include <k3bcddbresult.h>
#include <k3bmsf.h>
#include <k3bcddb.h>
#include <k3bcddbquery.h>
#include <k3btoolbox.h>
#include <kcutlabel.h>
#include <k3bstdguiitems.h>
#include <k3bapplication.h>
#include <cdinfo/k3bdiskinfodetector.h>
#include <k3bthememanager.h>
#include <k3baudiocdtrackdrag.h>

#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kstandarddirs.h>
#include <kdialogbase.h>

#include <qlayout.h>
#include <qheader.h>
#include <qlabel.h>
#include <qframe.h>
#include <qspinbox.h>
#include <qfont.h>
#include <qdragobject.h>


class K3bAudioCdView::AudioTrackViewItem : public K3bCheckListViewItem
{
public:
  AudioTrackViewItem( QListView* parent, 
		      QListViewItem* after,
		      int _trackNumber,
		      const K3b::Msf& length) 
    : K3bCheckListViewItem( parent, after ) {

    setText( 1, QString::number(_trackNumber).rightJustify( 2, ' ' ) );
    setText( 3, i18n("Track %1").arg(_trackNumber) );
    setText( 4, " " + length.toString() + " " );
    setText( 5, " " + KIO::convertSize( length.audioBytes() ) + " " );

    trackNumber = _trackNumber;

    setEditor( 2, LINE );
    setEditor( 3, LINE );

    setChecked(true);
  }

  void setup() {
    K3bCheckListViewItem::setup();
    
    setHeight( height() + 4 );
  }

  int trackNumber;

  void updateCddbData( const K3bCddbResultEntry& entry ) {
    setText( 2, entry.artists[trackNumber-1] );
    setText( 3, entry.titles[trackNumber-1] );
  }
};


K3bAudioCdView::K3bAudioCdView( QWidget* parent, const char *name )
  : K3bCdContentsView( true, parent, name )
{
  QGridLayout* mainGrid = new QGridLayout( mainWidget() );

  // toolbox
  // ----------------------------------------------------------------------------------
  QHBoxLayout* toolBoxLayout = new QHBoxLayout( 0, 0, 0, "toolBoxLayout" );
  m_toolBox = new K3bToolBox( mainWidget() );
  toolBoxLayout->addWidget( m_toolBox );
  QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
  toolBoxLayout->addItem( spacer );
  m_labelLength = new QLabel( mainWidget() );
  m_labelLength->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  toolBoxLayout->addWidget( m_labelLength );


  // the track view
  // ----------------------------------------------------------------------------------
  m_trackView = new K3bAudioCdListView( this, mainWidget() );

  connect( m_trackView, SIGNAL(itemRenamed(QListViewItem*, const QString&, int)),
	   this, SLOT(slotItemRenamed(QListViewItem*, const QString&, int)) );
  connect( m_trackView, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(slotContextMenu(KListView*, QListViewItem*, const QPoint&)) );
//   connect( m_trackView, SIGNAL(selectionChanged(QListViewItem*)), 
// 	   this, SLOT(slotTrackSelectionChanged(QListViewItem*)) );

  mainGrid->addLayout( toolBoxLayout, 0, 0 );
  mainGrid->addWidget( m_trackView, 1, 0 );


  m_cddb = new K3bCddb( this );

  connect( m_cddb, SIGNAL(infoMessage(const QString&)),
	   k3bappcore, SLOT(requestBusyInfo(const QString&)) );
  connect( m_cddb, SIGNAL(queryFinished(int)),
	   k3bappcore, SLOT(requestBusyFinish()) );
  connect( m_cddb, SIGNAL(queryFinished(int)),
	   this, SLOT(slotCddbQueryFinished(int)) );

  initActions();
  //  slotTrackSelectionChanged(0);

  setLeftPixmap( K3bTheme::MEDIA_LEFT );
  setRightPixmap( K3bTheme::MEDIA_AUDIO );
}


K3bAudioCdView::~K3bAudioCdView()
{
}


void K3bAudioCdView::setDisk( K3bDevice::DiskInfoDetector* did )
{
  m_toc = did->toc();
  m_device = did->device();

  // initialize cddb info for editing
  m_cddbInfo = K3bCddbResultEntry();
  m_cddbInfo.discid = QString::number( did->toc().discId(), 16 );

  for( int i = 0; i < (int)m_toc.count(); ++i ) {
    m_cddbInfo.titles.append("");
    m_cddbInfo.artists.append("");
    m_cddbInfo.extInfos.append("");
  }

  m_trackView->clear();
  enableInteraction(false);

  // create a listviewItem for every audio track
  int index = 1;
  for( K3bDevice::Toc::const_iterator it = m_toc.begin(); 
       it != m_toc.end(); ++it ) {

    // for now skip data tracks since we are not able to rip them to iso
    if( (*it).type() == K3bTrack::AUDIO ) {
      K3b::Msf length( (*it).length() );
      (void)new AudioTrackViewItem( m_trackView,
				    m_trackView->lastItem(),
				    index,
				    length );
    }

    index++;
  }

  m_cdText = did->cdText();
      
  reload();
}


void K3bAudioCdView::reload()
{
  // simulate a cddb entry with the cdtext data
  m_cddbInfo.cdTitle = m_cdText.title();
  m_cddbInfo.cdArtist = m_cdText.performer();
  m_cddbInfo.cdExtInfo = m_cdText.message();

  for( unsigned int i = 0; i < m_cdText.count(); ++i ) {
    m_cddbInfo.titles[i] = m_cdText[i].title();
    m_cddbInfo.artists[i] = m_cdText[i].performer();
    m_cddbInfo.extInfos[i] = m_cdText[i].message();
  }

  updateDisplay();

  KConfig* c = k3bcore->config();
  c->setGroup("Cddb");
  bool useCddb = ( c->readBoolEntry( "use local cddb query", true ) || 
		   c->readBoolEntry( "use remote cddb", false ) );

  if( useCddb &&
      ( m_cdText.isEmpty() ||
	KMessageBox::questionYesNo( this, 
				    i18n("Found Cd-Text. Do you want to use it instead of querying CDDB?"),
				    i18n("Found Cd-Text"), 
				    i18n("Use CD-Text"),
				    i18n("Query CDDB"),
				    "prefereCdTextOverCddb" ) == KMessageBox::No ) )
    queryCddb();
  else
    enableInteraction(true);
}


void K3bAudioCdView::initActions()
{
  m_actionCollection = new KActionCollection( this );

  KAction* actionSelectAll = new KAction( i18n("Check All"), 0, 0, this,
					  SLOT(slotCheckAll()), actionCollection(),
					  "check_all" );
  KAction* actionDeselectAll = new KAction( i18n("Uncheck All"), 0, 0, this,
					    SLOT(slotUncheckAll()), actionCollection(),
					    "uncheck_all" );
  KAction* actionSelect = new KAction( i18n("Check Track"), 0, 0, this,
				       SLOT(slotSelect()), actionCollection(),
				       "select_track" );
  KAction* actionDeselect = new KAction( i18n("Uncheck Track"), 0, 0, this,
					 SLOT(slotDeselect()), actionCollection(),
					 "deselect_track" );
  KAction* actionEditTrackCddbInfo = new KAction( i18n("Edit Track cddb Info"), "edit", 0, this,
						  SLOT(slotEditTrackCddb()), actionCollection(),
						  "edit_track_cddb" );
  KAction* actionEditAlbumCddbInfo = new KAction( i18n("Edit Album cddb Info"), "edit", 0, this,
						  SLOT(slotEditAlbumCddb()), actionCollection(),
						  "edit_album_cddb" );

  KAction* actionStartRip = new KAction( i18n("Start Ripping"), "cddarip", 0, this,
					 SLOT(startRip()), actionCollection(), "start_rip" );

  KAction* actionQueryCddb = new KAction( i18n("Query cddb"), "reload", 0, this,
					  SLOT(queryCddb()), actionCollection(), "query_cddb" );

  KAction* actionSaveCddbLocally = new KAction( i18n("Save Cddb Entry Locally"), "filesave", 0, this,
						SLOT(slotSaveCddbLocally()), actionCollection(), "save_cddb_local" );

  // TODO: set the actions tooltips and whatsthis infos

  // setup the popup menu
  m_popupMenu = new KActionMenu( actionCollection(), "popup_menu" );
  KAction* separator = new KActionSeparator( actionCollection(), "separator" );
  m_popupMenu->insert( actionSelect );
  m_popupMenu->insert( actionDeselect );
  m_popupMenu->insert( actionSelectAll );
  m_popupMenu->insert( actionDeselectAll );
  m_popupMenu->insert( separator );
  m_popupMenu->insert( actionEditTrackCddbInfo );
  m_popupMenu->insert( actionEditAlbumCddbInfo );
  m_popupMenu->insert( separator );
  m_popupMenu->insert( actionStartRip );

  // setup the toolbox
  m_toolBox->addButton( actionStartRip );
  m_toolBox->addSpacing();
  m_toolBox->addButton( actionQueryCddb );
  m_toolBox->addButton( actionSaveCddbLocally );
  m_toolBox->addButton( actionEditTrackCddbInfo );
  m_toolBox->addButton( actionEditAlbumCddbInfo );
}


void K3bAudioCdView::slotContextMenu( KListView*, QListViewItem*, const QPoint& p )
{
  m_popupMenu->popup(p);
}


void K3bAudioCdView::slotItemRenamed( QListViewItem* item, const QString& str, int col )
{
  AudioTrackViewItem* a = (AudioTrackViewItem*)item;
  if( col == 2 )
    m_cddbInfo.artists[a->trackNumber-1] = str;
  else if( col == 3 )
    m_cddbInfo.titles[a->trackNumber-1] = str;
  else if( col == 6 )
    m_cddbInfo.extInfos[a->trackNumber-1] = str;
}


void K3bAudioCdView::slotTrackSelectionChanged( QListViewItem* item )
{
  actionCollection()->action("edit_track_cddb")->setEnabled( item != 0 );
  actionCollection()->action("select_track")->setEnabled( item != 0 );
  actionCollection()->action("deselect_track")->setEnabled( item != 0 );
}


void K3bAudioCdView::startRip()
{
  QValueList<int> trackNumbers;
  for( QListViewItemIterator it( m_trackView ); it.current(); ++it ) {
    AudioTrackViewItem* a = (AudioTrackViewItem*)it.current();
    if( a->isChecked() )
      trackNumbers.append( a->trackNumber );
  }

  if( trackNumbers.count() == 0 ) {
    KMessageBox::error( this, i18n("Please select the tracks to rip."),
			i18n("No Tracks Selected") );
  }
  else {
    K3bAudioRippingDialog rip( m_toc,
			       m_device, 
			       m_cddbInfo, 
			       trackNumbers, 
			       this );
    rip.exec();
  }
}


void K3bAudioCdView::slotEditTrackCddb()
{
  QPtrList<QListViewItem> items( m_trackView->selectedItems() );
  if( !items.isEmpty() ) {
    AudioTrackViewItem* a = static_cast<AudioTrackViewItem*>(items.first());

    KDialogBase d( this, "trackCddbDialog", true, i18n("Cddb Track %1").arg(a->trackNumber),
		   KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true);
    QWidget* w = new QWidget( &d );

    KLineEdit* editTitle = new KLineEdit( m_cddbInfo.titles[a->trackNumber-1], w );
    KLineEdit* editArtist = new KLineEdit( m_cddbInfo.artists[a->trackNumber-1], w );
    KLineEdit* editExtInfo = new KLineEdit( m_cddbInfo.extInfos[a->trackNumber-1], w );
    QFrame* line = new QFrame( w );
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );

    QGridLayout* grid = new QGridLayout( w );
    grid->setSpacing( KDialog::spacingHint() );

    grid->addWidget( new QLabel( i18n("Title:"), w ), 0, 0 );
    grid->addWidget( editTitle, 0, 1 );
    grid->addMultiCellWidget( line, 1, 1, 0, 1 );
    grid->addWidget( new QLabel( i18n("Artist:"), w ), 2, 0 );
    grid->addWidget( editArtist, 2, 1 );
    grid->addWidget( new QLabel( i18n("Extra info:"), w ), 3, 0 );
    grid->addWidget( editExtInfo, 3, 1 );
    grid->setRowStretch( 4, 1 );

    d.setMainWidget(w);
    d.resize(260, d.sizeHint().height());

    if( d.exec() == QDialog::Accepted ) {
      m_cddbInfo.titles[a->trackNumber-1] = editTitle->text();
      m_cddbInfo.artists[a->trackNumber-1] = editArtist->text();
      m_cddbInfo.extInfos[a->trackNumber-1] = editExtInfo->text();
      a->updateCddbData( m_cddbInfo );
    }
  }
}


void K3bAudioCdView::slotEditAlbumCddb()
{
  KDialogBase d( this, "trackCddbDialog", true, i18n("Album Cddb"),
		 KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true);
  QWidget* w = new QWidget( &d );

  KLineEdit* editTitle = new KLineEdit( m_cddbInfo.cdTitle, w );
  KLineEdit* editArtist = new KLineEdit( m_cddbInfo.cdArtist, w );
  KLineEdit* editExtInfo = new KLineEdit( m_cddbInfo.cdExtInfo, w );
  KLineEdit* editGenre = new KLineEdit( m_cddbInfo.genre, w );
  QSpinBox* spinYear = new QSpinBox( 0, 9999, 1, w );
  spinYear->setValue( m_cddbInfo.year );
  QFrame* line = new QFrame( w );
  line->setFrameShape( QFrame::HLine );
  line->setFrameShadow( QFrame::Sunken );
  KComboBox* comboCat = new KComboBox( w );
  comboCat->insertStringList( K3bCddbQuery::categories() );

  // set the category
  for( int i = 0; i < comboCat->count(); ++i )
    if( comboCat->text(i) == m_cddbInfo.category ) {
      comboCat->setCurrentItem(i);
      break;
    }

  QGridLayout* grid = new QGridLayout( w );
  grid->setSpacing( KDialog::spacingHint() );

  grid->addWidget( new QLabel( i18n("Title:"), w ), 0, 0 );
  grid->addWidget( editTitle, 0, 1 );
  grid->addMultiCellWidget( line, 1, 1, 0, 1 );
  grid->addWidget( new QLabel( i18n("Artist:"), w ), 2, 0 );
  grid->addWidget( editArtist, 2, 1 );
  grid->addWidget( new QLabel( i18n("Extra info:"), w ), 3, 0 );
  grid->addWidget( editExtInfo, 3, 1 );
  grid->addWidget( new QLabel( i18n("Genre:"), w ), 4, 0 );
  grid->addWidget( editGenre, 4, 1 );
  grid->addWidget( new QLabel( i18n("Year:"), w ), 5, 0 );
  grid->addWidget( spinYear, 5, 1 );
  grid->addWidget( new QLabel( i18n("Category:"), w ), 6, 0 );
  grid->addWidget( comboCat, 6, 1 );
  grid->setRowStretch( 7, 1 );

  d.setMainWidget(w);
  d.resize(260, d.sizeHint().height());

  if( d.exec() == QDialog::Accepted ) {
    m_cddbInfo.cdTitle = editTitle->text();
    m_cddbInfo.cdArtist = editArtist->text();
    m_cddbInfo.cdExtInfo = editExtInfo->text();
    m_cddbInfo.category = comboCat->currentText();
    m_cddbInfo.genre = editGenre->text();
    m_cddbInfo.year = spinYear->value();

    updateDisplay();
  }
}


void K3bAudioCdView::queryCddb()
{
  KConfig* c = k3bcore->config();
  c->setGroup("Cddb");

  m_cddb->readConfig( c );

  if( c->readBoolEntry( "use local cddb query", true ) || 
      c->readBoolEntry( "use remote cddb", false ) ) {

    enableInteraction(false);
 
    m_cddb->query( m_toc );
  }
}


void K3bAudioCdView::slotCddbQueryFinished( int error )
{
  k3bappcore->requestBusyFinish();

  if( error == K3bCddbQuery::SUCCESS ) {
    m_cddbInfo = m_cddb->result();

    // save the entry locally
    KConfig* c = k3bcore->config();
    c->setGroup( "Cddb" );
    if( c->readBoolEntry( "save cddb entries locally", true ) )
      m_cddb->saveEntry( m_cddbInfo );

    //       kdDebug() << "cddb info:" << endl;
    //       kdDebug() << "DTITLE:  '" << m_cddbInfo.cdTitle << "'" << endl;
    //       kdDebug() << "DARTIST: '" << m_cddbInfo.cdArtist << "'" << endl;
    //       kdDebug() << "DEXT:    '" << m_cddbInfo.cdExtInfo << "'" << endl;
    //       kdDebug() << "DISCID:  '" << m_cddbInfo.discid << "'" << endl;

    //       for( QStringList::const_iterator it = m_cddbInfo.titles.begin();
    // 	   it != m_cddbInfo.titles.end(); ++it ) {
    // 	kdDebug() << "TTITLE:  '" << *it << "'" << endl;
    //       }
    //       for( QStringList::const_iterator it = m_cddbInfo.artists.begin();
    // 	   it != m_cddbInfo.artists.end(); ++it ) {
    // 	kdDebug() << "TARTIST: '" << *it << "'" << endl;
    //       }
    //       for( QStringList::const_iterator it = m_cddbInfo.extInfos.begin();
    // 	   it != m_cddbInfo.extInfos.end(); ++it ) {
    // 	kdDebug() << "TEXT:    '" << *it << "'" << endl;
    //       }

    updateDisplay();
  }
  else if( error == K3bCddbQuery::NO_ENTRY_FOUND ) {
    KMessageBox::information( this, i18n("No CDDB entry found."), i18n("CDDB") );
  }
  else {
    KMessageBox::information( this, m_cddb->errorString(), i18n("Cddb error") );
  }

  enableInteraction(true);
}


void K3bAudioCdView::slotSaveCddbLocally()
{
  // check if the minimal info has been inserted
  if( m_cddbInfo.category.isEmpty() ) {
    KMessageBox::sorry( this, i18n("Please set the category before saving.") );
    return;
  }
    
  if( m_cddbInfo.cdTitle.isEmpty() || m_cddbInfo.cdArtist.isEmpty() ) {
    KMessageBox::sorry( this, i18n("Please set CD artist and title before saving.") );
    return;
  }

  bool missingTitle = false;
  bool missingArtist = false;
  bool allTrackArtistsEmpty = true;
  for( unsigned int i = 0; i < m_cddbInfo.titles.count(); ++i ) {
    if( m_cddbInfo.titles[i].isEmpty() )
      missingTitle = true;
    if( m_cddbInfo.artists[i].isEmpty() )
      missingArtist = true;
    if( !m_cddbInfo.artists[i].isEmpty() )
      allTrackArtistsEmpty = false;
  }
  
  if( missingTitle ||
      ( missingArtist && !allTrackArtistsEmpty ) ) {
    KMessageBox::sorry( this, i18n("Please set at least artist and title on all tracks before saving.") );
    return;
  }

  // make sure the data gets updated (bad design like a lot in the cddb stuff! :(
  m_cddbInfo.rawData.truncate(0);

  KConfig* c = k3bcore->config();
  c->setGroup("Cddb");

  m_cddb->readConfig( c );

  m_cddb->saveEntry( m_cddbInfo );
  KMessageBox::information( this, 
			    i18n("Saved entry (%1) in category %2.")
			    .arg(m_cddbInfo.discid)
			    .arg(m_cddbInfo.category) );
}


void K3bAudioCdView::slotCheckAll()
{
  for( QListViewItemIterator it( m_trackView ); it.current(); ++it )
    ((AudioTrackViewItem*)it.current())->setChecked(true);
}

void K3bAudioCdView::slotUncheckAll()
{
  for( QListViewItemIterator it( m_trackView ); it.current(); ++it )
    ((AudioTrackViewItem*)it.current())->setChecked(false);
}

void K3bAudioCdView::slotSelect()
{
  QPtrList<QListViewItem> items( m_trackView->selectedItems() );
  for( QPtrListIterator<QListViewItem> it( items );
       it.current(); ++it )
    static_cast<AudioTrackViewItem*>(it.current())->setChecked(true);
}

void K3bAudioCdView::slotDeselect()
{
  QPtrList<QListViewItem> items( m_trackView->selectedItems() );
  for( QPtrListIterator<QListViewItem> it( items );
       it.current(); ++it )
    static_cast<AudioTrackViewItem*>(it.current())->setChecked(false);
}

void K3bAudioCdView::updateDisplay()
{
  // update the listview
  for( QListViewItemIterator it( m_trackView ); it.current(); ++it ) {
    AudioTrackViewItem* item = (AudioTrackViewItem*)it.current();
    item->updateCddbData( m_cddbInfo );
  }

  if( !m_cddbInfo.cdTitle.isEmpty() ) {
    QString s = m_cddbInfo.cdTitle;
    if( !m_cddbInfo.cdArtist.isEmpty() )
      s += " (" + m_cddbInfo.cdArtist + ")";
    setTitle( s );
  }
  else
    setTitle( i18n("Audio CD") );

  m_labelLength->setText( i18n("1 track (%1)", 
			       "%n tracks (%1)", 
			       m_toc.count()).arg(K3b::Msf(m_toc.length()).toString()) );
}


void K3bAudioCdView::enableInteraction( bool b )
{
  m_trackView->setEnabled(b);
  m_toolBox->setEnabled(b);
}


QDragObject* K3bAudioCdView::dragObject()
{
  QPtrList<QListViewItem> items = m_trackView->selectedItems();
  QValueList<int> tracks;
  for( QPtrListIterator<QListViewItem> it( items );
       it.current(); ++it )
    tracks.append( static_cast<AudioTrackViewItem*>(it.current())->trackNumber );

  if( !items.isEmpty() ) {
    QDragObject* drag = new K3bAudioCdTrackDrag( m_toc, 
						 tracks, 
						 m_cddbInfo,
						 m_device,
						 this );
    drag->setPixmap( m_trackView->createDragPixmap( items ) );
    return drag;
  }
  else
    return 0;
}

#include "k3baudiocdview.moc"
