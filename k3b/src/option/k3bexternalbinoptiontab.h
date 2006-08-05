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


#ifndef K3B_EXTERNALBIN_OPTIONTAB_H
#define K3B_EXTERNALBIN_OPTIONTAB_H

#include <qwidget.h>



class QPushButton;
class QListViewItem;
class KListView;
class K3bExternalBinManager;
class K3bExternalBinWidget;


class K3bExternalBinOptionTab : public QWidget
{
Q_OBJECT

 public:
  K3bExternalBinOptionTab( K3bExternalBinManager*, QWidget*, const char* name = 0 );
  ~K3bExternalBinOptionTab();

  void readSettings();
  void saveSettings();

 private:
  K3bExternalBinManager* m_manager;

  K3bExternalBinWidget* m_externalBinWidget;
};



#endif
