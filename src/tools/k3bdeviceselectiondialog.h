/***************************************************************************
                          k3bdeviceselectiondialog.h
                             -------------------
    begin                : Tue Nov 12 2002
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


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
