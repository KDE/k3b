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



#ifndef K3B_DEVICE_SELECTION_DIALOG_H
#define K3B_DEVICE_SELECTION_DIALOG_H


#include <kdialogbase.h>
#include "k3b_export.h"
#include <qptrlist.h>

namespace K3bDevice {
  class Device;
}


class LIBK3B_EXPORT K3bDeviceSelectionDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bDeviceSelectionDialog( QWidget* parent = 0, 
			    const char* name = 0, 
			    const QString& text = QString::null, 
			    bool modal = false );
  ~K3bDeviceSelectionDialog();

  void addDevice( K3bDevice::Device* );
  void addDevices( const QPtrList<K3bDevice::Device>& );

  void setSelectedDevice( K3bDevice::Device* );

  K3bDevice::Device* selectedDevice() const;

  static K3bDevice::Device* selectWriter( QWidget* parent, 
					  const QString& text = QString::null );
  static K3bDevice::Device* selectDevice( QWidget* parent, 
					  const QString& text = QString::null );
  static K3bDevice::Device* selectDevice( QWidget* parent, 
					  const QPtrList<K3bDevice::Device>& devices,
					  const QString& text = QString::null );

 private:
  class Private;
  Private* d;
};

#endif
