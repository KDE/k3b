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

#ifndef K3B_MIXED_VIEW_H
#define K3B_MIXED_VIEW_H

#include "../k3bview.h"

#include <kurl.h>

class K3bMixedDoc;
class QWidgetStack;
class K3bFillStatusDisplay;
class K3bDataFileView;
class K3bMixedDirTreeView;
class K3bAudioListView;
class QListViewItem;
class K3bDirItem;

class K3bMixedView : public K3bView
{
  Q_OBJECT

 public:
  K3bMixedView( K3bMixedDoc* doc, QWidget* parent = 0, const char* name = 0 );
  ~K3bMixedView();

  void burnDialog( bool withWritingButton = true );

  K3bDirItem* currentDir() const;

 private slots:
  void slotAudioTreeSelected();
  void slotDataTreeSelected();

 private:
  K3bMixedDoc* m_doc;

  QWidgetStack* m_widgetStack;

  K3bMixedDirTreeView* m_mixedDirTreeView;
  K3bDataFileView* m_dataFileView;
  K3bAudioListView* m_audioListView;

  K3bFillStatusDisplay* m_fillStatusDisplay;
};

#endif
