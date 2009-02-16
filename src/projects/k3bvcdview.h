/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*           (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
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

#ifndef K3BVCDVIEW_H
#define K3BVCDVIEW_H

#include <qstringlist.h>

// K3b Includes
#include <k3bstandardview.h>

class K3bVcdListView;
class QWidget;
class K3bVcdDoc;
class K3bProjectBurnDialog;

namespace K3b {
    class VcdProjectModel;
}

class K3bVcdView : public K3bStandardView
{
  Q_OBJECT
  
 public:
  K3bVcdView( K3bVcdDoc* pDoc, QWidget* parent );
  ~K3bVcdView();
  
 protected:
  K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

  void init();
  
 private:
  K3bVcdDoc* m_doc;
  K3b::VcdProjectModel* m_model;
  
  K3bVcdListView* m_vcdlist;
};

#endif
