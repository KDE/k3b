/***************************************************************************
                          k3bvcdview.h  -  description
                             -------------------
    begin                : Mon Nov 4 2002
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

#ifndef K3BVCDVIEW_H
#define K3BVCDVIEW_H

#include "../k3bview.h"

#include <qstringlist.h>
#include <qlist.h>


class K3bVcdListView;
class K3bVcdListViewItem;
class QWidget;
class K3bVcdDoc;
class K3bVcdTrack;
class QListViewItem;
class KListView;
class K3bFillStatusDisplay;
class K3bVcdBurnDialog;
class K3bProjectBurnDialog;


class K3bVcdView : public K3bView  {

  Q_OBJECT

 public:
  K3bVcdView( K3bVcdDoc* pDoc, QWidget* parent, const char *name = 0 );
  ~K3bVcdView();

  void burnDialog( bool );

 private:
  K3bVcdDoc* m_doc;

  K3bVcdListView* m_vcdlist;

  K3bFillStatusDisplay* m_fillStatusDisplay;
  K3bVcdBurnDialog* m_burnDialog;
};

#endif
