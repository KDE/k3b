/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3BDIVXOPTIONTAB_H_
#define _K3BDIVXOPTIONTAB_H_

#include <base_k3bdivxoptiontab.h>

class K3bDivxOptionTab : public base_K3bDivxOptions
{
  Q_OBJECT

 public:
  K3bDivxOptionTab(QWidget* parent,  const char* name );
  ~K3bDivxOptionTab();

 public slots:
  void readSettings();
  void saveSettings();
};

#endif
