/***************************************************************************
                          k3bsetup2fstabwidget.h  
                                   -
                       Widget to configure fstab entries
                             -------------------
    begin                : Sun Aug 25 13:19:59 CEST 2002
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

#ifndef K3BSETUP2_FSTAB_WIDGET_H
#define K3BSETUP2_FSTAB_WIDGET_H


#include "../k3bsetup2page.h"

#include <qmap.h>
#include <qstring.h>


class KConfig;
class K3bListView;
class QCheckBox;
class QListViewItem;
class K3bListView;
class K3bSetup2Task;


class K3bSetup2FstabWidget : public K3bSetup2Page
{
  Q_OBJECT

 public:
  K3bSetup2FstabWidget( K3bListView*, QWidget* parent = 0, const char* name = 0 );
  ~K3bSetup2FstabWidget();

 public slots:
  void load( KConfig* );
  bool save( KConfig* );

 private slots:
  void slotItemRenamed( QListViewItem*, const QString&, int );
  void clearTasks();
  void updateTasks();
  
 private:
   K3bListView* m_viewWithEntry;
   K3bListView* m_viewNoEntry;
   QCheckBox* m_checkCreateNewEntries;

   class FstabViewItem;

   QMap<FstabViewItem*, K3bSetup2Task*> m_tasks;
};


#endif
