/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_MOVIX_DVD_VIEW_H_
#define _K3B_MOVIX_DVD_VIEW_H_

#include <k3bmovixview.h>

class K3bMovixDvdDoc;


class K3bMovixDvdView : public K3bMovixView
{
  Q_OBJECT

 public:
  K3bMovixDvdView( K3bMovixDvdDoc* doc, QWidget *parent = 0, const char *name = 0 );
  ~K3bMovixDvdView();

 protected:
  K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0, const char* name = 0 );

 private:
  K3bMovixDvdDoc* m_doc;
};

#endif
