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


#ifndef K3B_DEVICE_OPTIONTAB_H
#define K3B_DEVICE_OPTIONTAB_H

#include <qwidget.h>
//Added by qt3to4:
#include <QLabel>

class QLabel;
class K3bDeviceWidget;


class K3bDeviceOptionTab : public QWidget
{
Q_OBJECT

 public:
  K3bDeviceOptionTab( QWidget* = 0 );
  ~K3bDeviceOptionTab();

  void readDevices();
  void saveDevices();

 private slots:
  void slotRefreshButtonClicked();

 private:
  QLabel*          m_labelDevicesInfo;
  K3bDeviceWidget* m_deviceWidget;
};



#endif
