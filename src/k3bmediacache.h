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


#ifndef _K3B_MEDIA_CACHE_H_
#define _K3B_MEDIA_CACHE_H_

#include <qobject.h>

#include <qvaluelist.h>

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3bcdtext.h>
#include <k3bdiskinfo.h>

#include "k3bmedium.h"

namespace K3bDevice {
  class DeviceManager;
}

class QCustomEvent;


/**
 * The Media Cache does know the status of all devices at all times
 * (except for blocked devices).
 *
 * It should be used to get information about media and device status
 * instead of the libk3bdevice methods for faster access.
 *
 * The Media Cache polls for new information every 2 seconds on all devices
 * (except for blocked ones) and emits signals in case a device status changed
 * (for example a media was inserted or removed).
 *
 * To start the media caching call buildDeviceList().
 */
class K3bMediaCache : public QObject
{
  Q_OBJECT

 public:
  K3bMediaCache( QObject* parent = 0 );
  ~K3bMediaCache();

  /**
   * block a device so it will not be polled. This is used
   * to disable polling on devices that are currently in use
   * for burning.
   *
   * \return A unique id to be used to unblock the device or -1 if the device
   *         is already blocked.
   */
  int blockDevice( K3bDevice::Device* dev );

  /**
   * Unblock a device that has been blocked with block() before.
   *
   * \param id The id returned by the previous call to block(). This makes
   *           sure only the one who did the block may unblock the device.
   *
   * \return true if dev has been blocked with id before. false otherwise.
   */
  bool unblockDevice( K3bDevice::Device* dev, int id );

  bool isBlocked( K3bDevice::Device* dev );

  /**
   * Read cached medium information.
   */
  K3bMedium medium( K3bDevice::Device* dev );

  /**
   * Read cached disk information.
   */
  K3bDevice::DiskInfo diskInfo( K3bDevice::Device* );

  /**
   * Read cached Table of contents.
   */
  K3bDevice::Toc toc( K3bDevice::Device* );

  /**
   * Read cached CD text from an Audio CD.
   */
  K3bDevice::CdText cdText( K3bDevice::Device* );

  /**
   * Read cached supported writing speeds.
   */
  QValueList<int> writingSpeeds( K3bDevice::Device* );

  /**
   * \see K3bMedium::shortString()
   */
  QString mediumString( K3bDevice::Device* device, bool useContent = true );

 signals:
  /**
   * Signal emitted whenever a medium changes. That means when a new medium is inserted
   * or an old one is removed.
   *
   * This signal will also be emitted when a previously blocked device becomes unblocked.
   *
   * Be aware though that the Media Cache will silently ignore removed devices. That means
   * once should also listen to K3bDevice::DeviceManager::changed() in case a USB drive or
   * something similar is removed.
   */
  void mediumChanged( K3bDevice::Device* dev );

 public slots:
  /**
   * Build the device list and start the polling.
   * It might make sense to connect this to K3bDevice::DeviceManager::changed()
   */
  void buildDeviceList( K3bDevice::DeviceManager* );

  /**
   * Clear the device list and stop all the polling.
   * This is also done in the destructor.
   */
  void clearDeviceList();

 private:
  class PollThread;
  class DeviceEntry;
  class MediaChangeEvent;

  QMap<K3bDevice::Device*, DeviceEntry*> m_deviceMap;

  DeviceEntry* findDeviceEntry( K3bDevice::Device* );
  void customEvent( QCustomEvent* );
};

#endif
