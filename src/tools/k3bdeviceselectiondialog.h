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



#ifndef K3B_DEVICE_SELECTION_DIALOG_H
#define K3B_DEVICE_SELECTION_DIALOG_H


#include <kdialogbase.h>

class QComboBox;
class K3bDevice;


class K3bDeviceSelectionDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bDeviceSelectionDialog( bool reader, bool writer, QWidget* parent = 0, 
			    const char* name = 0, const QString& text = "", bool modal = false );
  ~K3bDeviceSelectionDialog();

  K3bDevice* selectedDevice() const;

  static K3bDevice* selectWriter( QWidget* parent, const QString& text = "" );

 private:
  QComboBox* m_comboDevices;
};

#endif
