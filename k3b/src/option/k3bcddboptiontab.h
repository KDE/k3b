/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_CDDB_OPTIONTAB_H
#define K3B_CDDB_OPTIONTAB_H

#include "base_k3bcddboptiontab.h"


class K3bCddbOptionTab : public base_K3bCddbOptionTab
{
  Q_OBJECT

 public:
  K3bCddbOptionTab( QWidget* parent = 0, const char* name = 0 );
  ~K3bCddbOptionTab();

 public slots:
  void readSettings();
  void apply();

 private slots:
  void slotLocalDirAdd();
  void slotLocalDirRemove();
  void slotLocalDirDown();
  void slotLocalDirUp();

  void slotCddbServerAdd();
  void slotCddbServerRemove();
  void slotCddbServerDown();
  void slotCddbServerUp();

  void slotServerTypeChanged();

  void enDisableButtons();
};

#endif
