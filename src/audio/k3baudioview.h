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
#include <qmap.h>
#include <qlist.h>


class AudioListView;
class AudioListViewItem;
class QWidget;
class K3bAudioDoc;
class K3bAudioTrack;
class QDropEvent;
class QListViewItem;
class KListView;
class KPopupMenu;
class KAction;
class K3bFillStatusDisplay;
class K3bAudioBurnDialog;
class K3bProjectBurnDialog;
class QTimer;


/**
  *@author Sebastian Trueg
  */

class K3bAudioView : public K3bView  {

  Q_OBJECT
	
 public: 
  K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name = 0 );
  ~K3bAudioView();

  K3bProjectBurnDialog* burnDialog();
		
 private:
  void setupPopupMenu();
  void setupActions();

  K3bAudioDoc* m_doc;
	
  KAction* m_actionProperties;
  KAction* m_actionRemove;
  AudioListView* m_songlist;
  KPopupMenu* m_popupMenu;
  K3bFillStatusDisplay* m_fillStatusDisplay;
  K3bAudioBurnDialog* m_burnDialog;

  QTimer* m_displayRefreshTimer;

  QMap<K3bAudioTrack*, AudioListViewItem*> m_itemMap;
  QList<K3bAudioTrack> selectedTracks();

 signals:
  void dropped(const QStringList&, uint position);
  /** the item at position oldPos should be removed and reinserted at newPos */
  void itemMoved( uint oldPos, uint newPos );

 protected slots:
  void slotUpdateItems();

  /** generates a dropped signal */
  void slotDropped( KListView*, QDropEvent* e, QListViewItem* after );
  /** emits a signal to move an item */
  void slotItemMoved( QListViewItem*, QListViewItem*, QListViewItem* );
  void showPopupMenu( QListViewItem* _item, const QPoint& );
  void showPropertiesDialog();
  void slotRemoveTracks();
};

#endif
