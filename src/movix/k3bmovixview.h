/* 
 *
 * $Id: $
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



#ifndef _K3B_MOVIX_VIEW_H_
#define _K3B_MOVIX_VIEW_H_

#include <k3bview.h>

class K3bMovixDoc;
class K3bMovixListView;
class K3bFillStatusDisplay;
class KAction;
class KPopupMenu;
class QListViewItem;
class QPoint;


class K3bMovixView : public K3bView
{
  Q_OBJECT

 public:
  K3bMovixView( K3bMovixDoc* doc, QWidget* parent = 0, const char* name = 0 );
  ~K3bMovixView();

  void burnDialog( bool withWritingButton = true );

 private slots:
  void slotContextMenuRequested(QListViewItem*, const QPoint& , int );
  void slotRemoveItems();
  void slotRemoveSubTitleItems();
  void showPropertiesDialog();
  void slotAddSubTitleFile();

 private:
  K3bMovixListView* m_listView;
  K3bFillStatusDisplay* m_fillStatusDisplay;

  K3bMovixDoc* m_doc;

  KAction* m_actionProperties;
  KAction* m_actionRemove;
  KAction* m_actionRemoveSubTitle;
  KAction* m_actionAddSubTitle;
  KPopupMenu* m_popupMenu;
};

#endif
