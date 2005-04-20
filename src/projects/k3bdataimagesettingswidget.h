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

#ifndef K3B_DATAIMAGE_SETTINGS_WIDGET_H
#define K3B_DATAIMAGE_SETTINGS_WIDGET_H


#include "base_k3bdataimagesettings.h"

class K3bIsoOptions;


class K3bDataImageSettingsWidget : public base_K3bDataImageSettings
{
  Q_OBJECT

 public:
  K3bDataImageSettingsWidget( QWidget* parent = 0, const char* name =  0 );
  ~K3bDataImageSettingsWidget();

  void load( const K3bIsoOptions& );
  void save( K3bIsoOptions& );

 private slots:
  void slotJolietToggled( bool );
  void slotRockRidgeToggled( bool );
};


#endif
