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



#ifndef K3B_DEVICE_SELECTION_DIALOG_H
#define K3B_DEVICE_SELECTION_DIALOG_H


#include <kdialogbase.h>

#include <qptrlist.h>

namespace K3bCdDevice {
  class CdDevice;
}


class K3bDeviceSelectionDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bDeviceSelectionDialog( QWidget* parent = 0, 
			    const char* name = 0, 
			    const QString& text = QString::null, 
			    bool modal = false );
  ~K3bDeviceSelectionDialog();

  void addDevice( K3bCdDevice::CdDevice* );
  void addDevices( const QPtrList<K3bCdDevice::CdDevice>& );

  void setSelectedDevice( K3bCdDevice::CdDevice* );

  K3bCdDevice::CdDevice* selectedDevice() const;

  static K3bCdDevice::CdDevice* selectWriter( QWidget* parent, 
					      const QString& text = QString::null );
  static K3bCdDevice::CdDevice* selectDevice( QWidget* parent, 
					      const QPtrList<K3bCdDevice::CdDevice>& devices,
					      const QString& text = QString::null );

 private:
  class Private;
  Private* d;
};

#endif
