/***************************************************************************
                          audiolistview.h  -  description
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

#ifndef AUDIOLISTVIEW_H
#define AUDIOLISTVIEW_H


#include <klistview.h>

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

/**
  *@author Sebastian Trueg
  */
class K3bAudioListView : public KListView  
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
  KAction* m_actionPlay;
  KAction* m_actionPlayAll;
  KActionCollection* m_actionCollection;

  KPopupMenu* m_popupMenu;

  QMap<K3bAudioTrack*, K3bAudioListViewItem*> m_itemMap;

 private slots:
  void slotAnimation();
  void slotDropped( KListView*, QDropEvent* e, QListViewItem* after );
  void slotUpdateItems();
  void showPopupMenu( QListViewItem* item, const QPoint& );
  void showPropertiesDialog();
  void slotRemoveTracks();
  void slotPlaySelected();
  void slotPlayAll();

 protected:
  bool acceptDrag(QDropEvent* e) const;
  QDragObject* dragObject();
};

#endif
