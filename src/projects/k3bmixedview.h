/* 
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_MIXED_VIEW_H
#define K3B_MIXED_VIEW_H

#include <k3bstandardview.h>

#include <kurl.h>

class K3bMixedDoc;
class QStackedWidget ;
class K3bDataFileView;
class K3bMixedDirTreeView;
class K3bAudioTrackView;
class K3bDirItem;
class K3bAudioTrackPlayer;

namespace K3b
{
    class MixedProjectModel;
}

class K3bMixedView : public K3bStandardView
{
  Q_OBJECT

 public:
  K3bMixedView( K3bMixedDoc* doc, QWidget* parent = 0 );
  ~K3bMixedView();

  K3bDirItem* currentDir() const;

  K3bAudioTrackPlayer* player() const;

 public Q_SLOTS:
  void slotBurn();
  void addUrls( const KUrl::List& );

 protected:
  K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

 private Q_SLOTS:
  void slotAudioTreeSelected();
  void slotDataTreeSelected();

 private:
  K3bMixedDoc* m_doc;
  K3b::MixedProjectModel* m_model;
};

#endif
