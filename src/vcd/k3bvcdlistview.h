/***************************************************************************
                          k3bvcdlistview.h  -  description
                             -------------------
    begin                : Sam Nov 9 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#ifndef K3BVCDLISTVIEW_H
#define K3BVCDLISTVIEW_H


#include "../tools/k3blistview.h"

#include <qmap.h>

class QDragEnterEvent;
class QDragObject;
class QDropEvent;
class QTimer;
class KPopupMenu;
class KAction;
class K3bVcdDoc;
class K3bView;
class K3bVcdTrack;
class KActionCollection;
class K3bVcdListViewItem;
class QPainter;


class K3bVcdListView : public K3bListView
{
  Q_OBJECT

 public:
  K3bVcdListView( K3bView*, K3bVcdDoc*, QWidget *parent=0, const char *name=0);
  ~K3bVcdListView();

  /**
   * reimplemented from KListView
   */
  void insertItem( QListViewItem* );

  KActionCollection* actionCollection() const { return m_actionCollection; }

  QPtrList<K3bVcdTrack> selectedTracks();

 signals:
  void lengthReady();

 private:
  void setupColumns();
  void setupPopupMenu();
  void setupActions();

  K3bVcdDoc* m_doc;
  K3bView* m_view;

  KAction* m_actionProperties;
  KAction* m_actionRemove;
  KActionCollection* m_actionCollection;

  KPopupMenu* m_popupMenu;

  QMap<K3bVcdTrack*, K3bVcdListViewItem*> m_itemMap;

 private slots:
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
