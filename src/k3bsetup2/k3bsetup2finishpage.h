/* 
 *
 * $Id: $
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

/***************************************************************************
                          k3bsetup2finishpage.h  
                                   -
                  Just some text and a workindicator
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

#ifndef K3BSETUP2_FINISH_PAGE_H
#define K3BSETUP2_FINISH_PAGE_H

#include "k3bsetup2page.h"

class K3bBusyWidget;


class K3bSetup2FinishPage : public K3bSetup2Page
{
  Q_OBJECT

 public:
  K3bSetup2FinishPage( QWidget* parent = 0, const char* name = 0 );
  ~K3bSetup2FinishPage();

  void showBusy( bool );

 private:
  K3bBusyWidget* m_busyWidget;
};

#endif
