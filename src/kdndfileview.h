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


#ifndef KDNDFILEVIEW_H
#define KDNDFILEVIEW_H

#include <kfiledetailview.h>
#include <kfileiconview.h>

class QMouseEvent;


/**
 * adds Drag'n'Drop support
  * @author Sebastian Trueg
  */
class KDndFileDetailView : public KFileDetailView
{
  Q_OBJECT

 public: 
  KDndFileDetailView( QWidget* parent, const char* name );
  ~KDndFileDetailView();
  
 protected:
  QDragObject* dragObject();
};


class KDndFileIconView : public KFileIconView
{
  Q_OBJECT

 public:
  KDndFileIconView( QWidget* parent = 0, const char* name = 0);
  ~KDndFileIconView();

 protected:
  QDragObject* dragObject();
};

#endif
