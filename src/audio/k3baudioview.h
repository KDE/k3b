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

#include "k3bview.h"


class AudioListView;
class QWidget;
class K3bAudioDoc;
class K3bAudioTrack;
class QDropEvent;
class QListViewItem;
class KListView;


/**
  *@author Sebastian Trueg
  */

class K3bAudioView : public K3bView  {

	Q_OBJECT
	
public: 
	K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name = 0, int wflags = 0 );
	~K3bAudioView();
	
private:
	AudioListView* m_songlist;

signals:
 	void dropped(const QString&, uint position);
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
};

#endif
