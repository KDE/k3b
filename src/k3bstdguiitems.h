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

#ifndef K3B_STD_GUIITEMS_H
#define K3B_STD_GUIITEMS_H


class QWidget;
class QCheckBox;
class QComboBox;


class K3bStdGuiItems
{
 public:
  static QCheckBox* simulateCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* daoCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* burnproofCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* onlyCreateImagesCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* removeImagesCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* onTheFlyCheckbox( QWidget* parent = 0, const char* name = 0 );
  static QCheckBox* cdTextCheckbox( QWidget* parent = 0, const char* name = 0);
  static QComboBox* paranoiaModeComboBox( QWidget* parent = 0, const char* name = 0 );
  static QComboBox* dataModeComboboxBox( QWidget* parent = 0, const char* name = 0 );

 private:
  K3bStdGuiItems() {}
};

#endif
