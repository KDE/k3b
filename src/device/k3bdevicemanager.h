/***************************************************************************
                          k3bdevicemanager.h  -  description
                             -------------------
    begin                : Sun Mar 17 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#ifndef K3BDEVICEMANAGER_H
#define K3BDEVICEMANAGER_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmemarray.h>
#include <qptrlist.h>

#include <kdebug.h>
#include "k3bdevice.h"

class KProcess;
class KConfig;
class K3bExternalBinManager;
class cdrom_drive;


class K3bDeviceManager : public QObject 
{
  Q_OBJECT

 public:
  ~K3bDeviceManager();

  static K3bDeviceManager* self();

  K3bDevice* deviceByName( const QString& );

  K3bDevice* findDevice( int bus, int id, int lun );
  K3bDevice* findDevice( const QString& devicename );

  /**
   * Before getting the devices do a @ref scanbus().
   * @return List of all writer devices.
   */
  QList<K3bDevice>& burningDevices();

  /**
   * Note that all burning devices can also be used as
   * reading device and are not present in this list.
   * Before getting the devices do a @ref scanbus().
   * @return List of all reader devices without writer devices.
   **/
  QList<K3bDevice>& readingDevices();

  QList<K3bDevice>& allDevices();


  /** writes to stderr **/
  void printDevices();

  /**
   * Returns number of found devices and constructs
   * the lists m_burner and m_reader.
   **/
  int scanbus();

  void scanFstab();

  /**
   * Reads the device information from the config file.
   */
  bool readConfig( KConfig* );

  bool saveConfig( KConfig* );

  /**
   * Clears the writers and readers list of devices.
   */
  void clear();

  /**
   * add a new device like "/dev/mebecdrom" to be sensed
   * by the deviceManager.
   */
  K3bDevice* addDevice( const QString& );
  
 private slots:
  void slotCollectStdout( KProcess*, char* data, int len );

 private:
  /**
   * Constructs a device-manager and scans the scsi-bus
   * for devices. Every instance of K3bDeviceManager on
   * a machine is equal, so having multible instances
   * does not make sense.
   **/
  K3bDeviceManager();
  
  bool testForCdrom( const QString& );
  K3bDevice::interface determineInterfaceType(const QString&);
  bool determineBusIdLun( const QString &dev, int& bus, int& id, int& lun );
  void determineCapabilities(K3bDevice *dev);
  QString resolveSymLink( const QString& path );

  K3bExternalBinManager* m_externalBinManager;

  QList<K3bDevice> m_reader;
  QList<K3bDevice> m_writer;
  QList<K3bDevice> m_allDevices;;
  int m_foundDevices;

  QString m_processOutput;

  K3bDevice* initializeScsiDevice( const QString& devname);
  K3bDevice* initializeIdeDevice( const QString& devname);
};

#endif
