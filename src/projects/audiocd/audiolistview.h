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


#ifndef AUDIOLISTVIEW_H
#define AUDIOLISTVIEW_H


#include <k3blistview.h>

#include <qmap.h>

class QDragEnterEvent;
class QDragObject;
class QDropEvent;
class QTimer;
class KPopupMenu;
class KAction;
class K3bAudioDoc;
class K3bView;
class K3bAudioTrack;
class KActionCollection;
class K3bAudioListViewItem;
class QPainter;


/**
  *@author Sebastian Trueg
  */
class K3bAudioListView : public K3bListView  
{
  Q_OBJECT

 public:
  K3bAudioListView( K3bView*, K3bAudioDoc*, QWidget *parent=0, const char *name=0);
  ~K3bAudioListView();

  /**
   * reimplemented from KListView
   */
  void insertItem( QListViewItem* );

  KActionCollection* actionCollection() const { return m_actionCollection; }

  QPtrList<K3bAudioTrack> selectedTracks();

 signals:
  void lengthReady();

 private:
  void setupColumns();
  void setupPopupMenu();
  void setupActions();

  QTimer* m_animationTimer;

  K3bAudioDoc* m_doc;
  K3bView* m_view;

  KAction* m_actionProperties;
  KAction* m_actionRemove;
  KActionCollection* m_actionCollection;

  KPopupMenu* m_popupMenu;

  QMap<K3bAudioTrack*, K3bAudioListViewItem*> m_itemMap;

 private slots:
  void slotAnimation();
  void slotDropped( KListView*, QDropEvent* e, QListViewItem* after );
  void slotUpdateItems();
  void showPopupMenu( KListView*, QListViewItem* item, const QPoint& );
  void showPropertiesDialog();
  void slotRemoveTracks();

 protected:
  bool acceptDrag(QDropEvent* e) const;
  QDragObject* dragObject();
};

#endif
