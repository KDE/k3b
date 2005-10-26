/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_MEDIA_SELECTION_COMBOBOX_H_
#define _K3B_MEDIA_SELECTION_COMBOBOX_H_

#include <kcombobox.h>

#include "k3bmedium.h"

namespace K3bDevice {
  class Device;
  class DeviceManager;
}

/**
 * Combo box which allows to select a media (in comparision
 * to the K3bDeviceComboBox which allows to select a device.
 *
 * This class uses the K3bMediaCache to update it's status.
 */
class K3bMediaSelectionComboBox : public KComboBox
{
  Q_OBJECT

 public:
  K3bMediaSelectionComboBox( QWidget* parent );
  virtual ~K3bMediaSelectionComboBox();

  /**
   * Although the widget allows selection of media this
   * results in a device being selected.
   */
  K3bDevice::Device* selectedDevice() const;

 signals:
  /**
   * Be aware that his signal will also be emitted in case
   * no medium is available with a null pointer.
   */
  void selectionChanged( K3bDevice::Device* );

 public slots:
  /**
   * Only works in case the device actually contains a usable medium.
   * Otherwise the currently selected medium stays selected.
   */
  void setSelectedDevice( K3bDevice::Device* );

  /**
   * Set the wanted medium type. Defaults to writable CD.
   *
   * \param type a bitwise combination of the K3bDevice::MediaType enum
   */
  void setWantedMediumType( int type );

  /**
   * Set the wanted medium state. Defaults to empty media.
   *
   * \param state a bitwise combination of the K3bDevice::State enum
   */
  void setWantedMediumState( int state );

 private slots:
  void slotMediumChanged( K3bDevice::Device* );
  void slotDeviceManagerChanged( K3bDevice::DeviceManager* );
  void slotActivated( int i );
  void slotUpdateToolTip( K3bDevice::Device* );

 protected:
  void updateMedia();
  virtual bool showMedium( const K3bMedium& );
  virtual QString mediumString( const K3bMedium& );
  virtual QString mediumToolTip( const K3bMedium& );

 private:
  void updateMedium( K3bDevice::Device* );
  void addMedium( K3bDevice::Device* );
  void showNoMediumMessage();
  void clear();

  // usedby the tooltip
  K3bDevice::Device* deviceAt( unsigned int index );

  class ToolTip;
  friend class ToolTip;

  class Private;
  Private* d;
};

#endif
