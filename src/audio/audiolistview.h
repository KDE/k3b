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

class QDragEnterEvent;
class QTimer;


/**
  *@author Sebastian Trueg
  */
class K3bAudioListView : public KListView  
{
  Q_OBJECT

 public:
  K3bAudioListView(QWidget *parent=0, const char *name=0);
  ~K3bAudioListView();

  /**
   * reimplemented from KListView
   */
  void insertItem( QListViewItem* );

 private:
  void setupColumns();

  QTimer* m_animationTimer;

 private slots:
  void slotAnimation();

 protected:
  bool acceptDrag(QDropEvent* e) const;
};

#endif
