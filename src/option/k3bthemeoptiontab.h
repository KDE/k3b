/* 
 *
 * $Id$
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


#ifndef _K3BTHEMEOPTIONTAB_H_
#define _K3BTHEMEOPTIONTAB_H_

#include "base_k3bthemeoptiontab.h"


/**
  *@author Sebastian Trueg
  */
class K3bThemeOptionTab : public base_K3bThemeOptionTab
{
  Q_OBJECT

 public:
  K3bThemeOptionTab( QWidget* parent = 0, const char* name = 0 );
  ~K3bThemeOptionTab();

  void readSettings();
  bool saveSettings();

 private slots:
  void selectionChanged();

 private:
  class Private;
  Private* d;
};

#endif
