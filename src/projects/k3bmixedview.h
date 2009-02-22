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

namespace K3b {
    class MixedDoc;
}
class QStackedWidget ;
namespace K3b {
    class DataFileView;
}
namespace K3b {
    class MixedDirTreeView;
}
namespace K3b {
    class AudioTrackView;
}
namespace K3b {
    class DirItem;
}
namespace K3b {
    class AudioTrackPlayer;
}

namespace K3b
{
    class MixedProjectModel;
}

namespace K3b {
class MixedView : public StandardView
{
  Q_OBJECT

 public:
  MixedView( MixedDoc* doc, QWidget* parent = 0 );
  ~MixedView();

  DirItem* currentDir() const;

  AudioTrackPlayer* player() const;

 public Q_SLOTS:
  void slotBurn();
  void addUrls( const KUrl::List& );

 protected:
  ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

 private Q_SLOTS:
  void slotAudioTreeSelected();
  void slotDataTreeSelected();

 private:
  MixedDoc* m_doc;
  K3b::MixedProjectModel* m_model;
};
}

#endif
