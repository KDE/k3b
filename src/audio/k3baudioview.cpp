/***************************************************************************
                          k3baudioview.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2001
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

#include "../k3b.h"
#include "k3baudioview.h"
#include "k3baudiodoc.h"
#include "audiolistview.h"
#include "audiolistviewitem.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackdialog.h"
#include "k3baudioburndialog.h"
#include "../k3bfillstatusdisplay.h"

// QT-includes
#include <qlayout.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qdragobject.h>
#include <qpoint.h>

// KDE-includes
#include <kpopupmenu.h>
#include <kaction.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kapp.h>


K3bAudioView::K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name, int wflags )
 : K3bView( pDoc, parent, name, wflags )
{
	QGridLayout* grid = new QGridLayout( this );
	grid->setSpacing( 5 );
	grid->setMargin( 2 );
	
	m_songlist = new AudioListView( this );
	m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );
	m_fillStatusDisplay->showTime();
	m_propertiesDialog = 0;
	m_burnDialog = 0;
	
	grid->addWidget( m_songlist, 0, 0 );
	grid->addWidget( m_fillStatusDisplay, 1, 0 );

	setupPopupMenu();
		
	// TODO: create slot dropped that calculates the position where was dropped and passes it to the signal dropped( KURL&, int)
	connect( m_songlist, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*)),
					this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*)) );
	connect( m_songlist, SIGNAL(moved(QListViewItem*,QListViewItem*,QListViewItem*)),
					this, SLOT(slotItemMoved( QListViewItem*, QListViewItem*, QListViewItem* )) );
	
	connect( m_songlist, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
					this, SLOT(showPopupMenu(QListViewItem*, const QPoint&)) );
	connect( m_songlist, SIGNAL(clicked(QListViewItem*)),
					this, SLOT(itemClicked(QListViewItem*)) );
}

K3bAudioView::~K3bAudioView(){
}


K3bProjectBurnDialog* K3bAudioView::burnDialog()
{
	if( !m_burnDialog )
		m_burnDialog = new K3bAudioBurnDialog( (K3bAudioDoc*)getDocument(), k3bMain(), "audioburndialog", true );
		
	return m_burnDialog;
}


void K3bAudioView::setupPopupMenu()
{
	m_popupMenu = new KPopupMenu( m_songlist, "AudioViewPopupMenu" );
	m_popupMenu->insertTitle( i18n( "Track Options" ) );
	actionProperties = new KAction( i18n("&Properties"), SmallIcon( "edit" ), CTRL+Key_P, this, SLOT(showPropertiesDialog()), this );
	actionRemove = new KAction( i18n( "&Remove" ), SmallIcon( "editdelete" ), Key_Delete, this, SLOT(removeTrack()), this );
	actionRemove->plug( m_popupMenu );
	actionProperties->plug( m_popupMenu);
}


void K3bAudioView::addItem( K3bAudioTrack* _track )
{
	qDebug( "(K3bAudioView) adding new item to list: " + _track->fileName() );
	(void)new AudioListViewItem( _track, m_songlist );
	m_fillStatusDisplay->repaint();
}

void K3bAudioView::slotDropped( KListView*, QDropEvent* e, QListViewItem* after )
{
	if( !e->isAccepted() )
		return;

	QString droppedText;
	QTextDrag::decode( e, droppedText );
	QStringList _urls = QStringList::split("\r\n", droppedText );
	
	AudioListViewItem* _item = (AudioListViewItem*)after;
	uint _pos;
	if( _item == 0L )
		_pos = 0;
	else
		_pos = _item->text(0).toInt();
		
	emit dropped( _urls, _pos );
}

void K3bAudioView::slotItemMoved( QListViewItem* item, QListViewItem*, QListViewItem* afterNow )
{
	if( !item)
		return;
		
	AudioListViewItem *_item, *_now;
	_item = (AudioListViewItem*)item;
	_now = (AudioListViewItem*)afterNow;
	
	uint before, after;
	// text starts at 1 but QList starts at 0
	before = _item->text(0).toInt()-1;
	if( _now ) {
		after = _now->text(0).toInt()-1;
		if( before > after )
			after++;
	}
	else
		after = 0;
	
	emit itemMoved( before, after );
}

void K3bAudioView::showPopupMenu( QListViewItem* _item, const QPoint& _point )
{
	if( _item )
		m_popupMenu->popup( _point );
}

void K3bAudioView::itemClicked( QListViewItem* _item )
{
	if( m_propertiesDialog ) {
		if( m_propertiesDialog->sticky() )
			updatePropertiesDialog( _item );
		else
			m_propertiesDialog->hide();
	}
}

void K3bAudioView::updatePropertiesDialog( QListViewItem* _item )
{
	if( m_propertiesDialog && m_propertiesDialog->isVisible() ) {
		AudioListViewItem* _sel = (AudioListViewItem*)_item;
		if( _sel )
			m_propertiesDialog->setTrack( _sel->audioTrack() );
	}
}

void K3bAudioView::showPropertiesDialog()
{
	if( !m_propertiesDialog ) {
		// get an instance from K3bApp
		m_propertiesDialog = k3bMain()->audioTrackDialog();
		connect( m_songlist, SIGNAL(itemRenamed(QListViewItem*)), m_propertiesDialog, SLOT(updateView()) );
	}		
	if( !m_propertiesDialog->isVisible() )
		m_propertiesDialog->show();
	
	updatePropertiesDialog( m_songlist->selectedItem() );
}

void K3bAudioView::removeTrack()
{
	AudioListViewItem* _track = (AudioListViewItem*)m_songlist->selectedItem();
	if( _track ) {
		((K3bAudioDoc*)doc)->removeTrack( _track->audioTrack()->index() );
		
		// not best, I think we should connect to doc.removedTrack (but while there is only one view this is not important!)
		delete _track;
	}
}
