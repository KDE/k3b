/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_MEDIA_SELECTION_COMBOBOX_H_
#define _K3B_MEDIA_SELECTION_COMBOBOX_H_

#include <kcombobox.h>

#include "k3bmedium.h"

namespace K3bDevice {
  class Device;
  class DeviceManager;
}

/**
 * Combo box which allows to select a media (in comparison 
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

  QValueList<K3bDevice::Device*> allDevices() const;

  int wantedMediumType() const;
  int wantedMediumState() const;
  int wantedMediumContent() const;

 signals:
  /**
   * Be aware that his signal will also be emitted in case
   * no medium is available with a null pointer.
   */
  void selectionChanged( K3bDevice::Device* );

  /**
   * This signal is emitted if the selection of media changed.
   * This includes a change due to changing the wanted medium state.
   */
  void newMedia();

  void newMedium( K3bDevice::Device* dev );

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

  /**
   * Set the wanted medium content type. The default is K3bMedium::CONTENT_ALL (i.e. ignore media 
   * content)
   * Be aware that 0 maps to K3bMedium::CONTENT_NONE, i.e. empty media.
   *
   * \param content A bitwise or of K3bMedium::MediumContent
   */
  void setWantedMediumContent( int content );

  /**
   * Set the device to ignore. This device will not be checked for
   * wanted media. This is many useful for media copy.
   *
   * \param dev The device to ignore or 0 to not ignore any device.
   */
  void setIgnoreDevice( K3bDevice::Device* dev );

 private slots:
  void slotMediumChanged( K3bDevice::Device* );
  void slotDeviceManagerChanged( K3bDevice::DeviceManager* );
  void slotActivated( int i );
  void slotUpdateToolTip( K3bDevice::Device* );

 protected:
  void updateMedia();
  virtual bool showMedium( const K3bMedium& ) const;
  virtual QString mediumString( const K3bMedium& ) const;
  virtual QString mediumToolTip( const K3bMedium& ) const;
  virtual QString noMediumMessage() const;

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
