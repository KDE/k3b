/***************************************************************************
                          k3baudioview.h  -  description
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

#ifndef K3BAUDIOVIEW_H
#define K3BAUDIOVIEW_H

#include "../k3bview.h"

#include <qstringlist.h>

class AudioListView;
class QWidget;
class K3bAudioDoc;
class K3bAudioTrack;
class QDropEvent;
class QListViewItem;
class KListView;
class KPopupMenu;
class KAction;
class K3bAudioTrackDialog;
class K3bFillStatusDisplay;


/**
  *@author Sebastian Trueg
  */

class K3bAudioView : public K3bView  {

	Q_OBJECT
	
public: 
	K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name = 0, int wflags = 0 );
	~K3bAudioView();
	
private:
	void setupPopupMenu();
	
	KAction* actionProperties;
	KAction* actionRemove;
	AudioListView* m_songlist;
	KPopupMenu* m_popupMenu;
	K3bAudioTrackDialog* m_propertiesDialog;
	K3bFillStatusDisplay* m_fillStatusDisplay;
	
signals:
 	void dropped(const QStringList&, uint position);
	 /** the item at position oldPos should be removed and reinserted at newPos */
	void itemMoved( uint oldPos, uint newPos );

public slots:
  /** adds a new item for _track */
  void addItem( K3bAudioTrack* _track );

protected slots:
	/** generates a dropped signal */
	void slotDropped( KListView*, QDropEvent* e, QListViewItem* after );
	/** emits a signal to move an item */
	void slotItemMoved( QListViewItem*, QListViewItem*, QListViewItem* );
	void showPopupMenu( QListViewItem* _item, const QPoint& );
	void showPropertiesDialog();
	void itemClicked( QListViewItem* _item );
	void updatePropertiesDialog( QListViewItem* _item );
	void removeTrack();
};

#endif
