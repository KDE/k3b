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


#ifndef K3BDEVICE_H
#define K3BDEVICE_H

#include <qstringlist.h>
#include "k3bdiskinfo.h"

#include "k3bmsf.h"

#define SUPPORT_IDE

namespace K3bCdDevice
{

  class Toc;

  class CdDevice
  {

  public:
    /**
     * The available cdrdao drivers
     */
    static const char* cdrdao_drivers[];

    enum interface { SCSI, IDE, OTHER };
    enum DeviceType    { CDR = 1,
                         CDRW = 2,
                         CDROM = 4,
                         DVDROM = 8,
                         DVDRAM = 16,
                         DVDR = 32,
                         DVDRW = 64,
                         DVDPR = 128,
                         DVDPRW = 256 };
    enum DiskStatus { EMPTY = 0,
                      APPENDABLE = 1,
                      COMPLETE = 2,
                      NO_DISK = -1,
                      NO_INFO = -2 };
    enum WriteMode { SAO = 1,
                     TAO = 2,
                     PACKET = 4,
                     SAO_R96P = 8,
                     SAO_R96R = 16,
                     RAW_R16 = 32,
                     RAW_R96P = 64,
                     RAW_R96R = 128 };


    class Private
    {
    public:
      Private() {}

      QString blockDeviceName;
      QString genericDevice;
      int deviceType;
      interface interfaceType;
      QString mountPoint;
      QString mountDeviceName;
      QStringList allNodes;
    };

    /**
      * create a K3bDevice from a cdrom_drive struct
      * (cdparanoia-lib)
      */
    CdDevice( const QString& devname );
    ~CdDevice();

    bool init();

    interface interfaceType();
    int type() const;

    const QString& vendor() const { return m_vendor; }
    const QString& description() const { return m_description; }
    const QString& version() const { return m_version; }
    bool           burner() const { return m_burner; }
    bool           writesCdrw() const { return m_bWritesCdrw; }
    bool           burnproof() const { return m_burnproof; }
    bool           dao() const { return m_dao; }
    int            maxReadSpeed() const { return m_maxReadSpeed; }
    int            currentWriteSpeed() const { return m_currentWriteSpeed; }

    int            bufferSize() const { return m_bufferSize; }

    /**
     * ioctlDevice is returned
     */
    const QString& devicename() const;

    /**
     * compatibility
     */
    const QString& genericDevice() const;

    /**
     * returnes blockDeviceName()
     */
    const QString& ioctlDevice() const;

    /**
     * for SCSI devices this should be something like /dev/scd0 or /dev/sr0
     * for ide device this should be something like /dev/hdb1
     */
    const QString& blockDeviceName() const;

    /**
     * returnes all device nodes for this drive
     * this mainly makes sense with scsi devices which
     * can be accessed through /dev/scdX or /dev/srX
     * and is useful for fstab scanning
     */
    const QStringList& deviceNodes() const;

    void addDeviceNode( const QString& );

    const QString& mountDevice() const;

    /** makes only sense to use with sg devices */
    QString busTargetLun() const;
    int scsiBus() const { return m_bus; }
    int scsiId() const { return m_target; }
    int scsiLun() const { return m_lun; }

    int            maxWriteSpeed() const { return m_maxWriteSpeed; }
    const QString& cdrdaoDriver() const { return m_cdrdaoDriver; }

    const QString& mountPoint() const;

    /**
     * returns: 0 auto (no cdrdao-driver selected)
     *          1 yes
     *          2 no
     */
    int cdTextCapable() const;


    /** internally K3b value. */
    void setCurrentWriteSpeed( int s ) { m_currentWriteSpeed = s; }


    void setIsWriter( bool b ) { m_burner = b; }

    /**
     * Use this if the speed was not detected correctly.
     */
    void setMaxReadSpeed( int s ) { m_maxReadSpeed = s; }

    /**
     * Use this if the speed was not detected correctly.
     */
    void setMaxWriteSpeed( int s ) { m_maxWriteSpeed = s; }

    /**
     * Use this if cdrdao is not able to autodetect the nessessary driver.
     */
    void setCdrdaoDriver( const QString& d ) { m_cdrdaoDriver = d; }

    /**
     * Only used if the cdrdao-driver is NOT set to "auto".
     * In that case it must be manually set because there
     * is no way to autosense the cd-text capability.
     */
    void setCdTextCapability( bool );

    void setBurnproof( bool );
    void setWritesCdrw( bool b ) { m_bWritesCdrw = b; }
    void setDao( bool b ) { m_dao = b; }
    void setBufferSize( int b ) { m_bufferSize = b; }

    void setMountPoint( const QString& );
    void setMountDevice( const QString& d );

    /** checks if unit is ready, returns:
     * <ul>
     *  <li>0: OK</li>
     *  <li>1: scsi command failed</li>
     *  <li>2: not ready</li>
     *  <li>3: not ready, no disk in drive</li>
     *  <li>4: not ready, tray out</li>
     * </ul>
     */
    int isReady() const;

    /** not quite sure if this is nessesary! */
    //  virtual bool rezero() const;

    /**
     * Saves the cd's total number of available blocks in "length"
     * returns false if capacity could not be retrieved.
     * DOES NOT WORK SO FAR!
     */
    /*   bool cdCapacity( long* ); */


    /**
     *  checks if disk is empty, returns:
     *  <li> 0: disk is empty</li>
     *  <li> 1: disk is not empty, but appendable</li>
     *  <li> 2: disk is complete</li>
     *  <li>-1: not ready, no disk in drive</li>
     * </ul>
     */
    int isEmpty();

    bool rewritable();

    bool isDVD();
    DiskInfo::type diskType();
    int numSessions();
    K3b::Msf discSize();
    K3b::Msf remainingSize();

    /**
     * block or unblock the drive's tray
     * returns true on success and false on scsi-error
     */
    bool block( bool ) const;

    void eject() const;
    void load() const;

    bool supportsWriteMode( WriteMode );

  protected:
    bool furtherInit();

    QString m_vendor;
    QString m_description;
    QString m_version;
    bool m_burner;
    bool m_bWritesCdrw;
    bool m_burnproof;
    bool m_dao;
    QString m_cdrdaoDriver;
    int m_cdTextCapable;
    int m_maxReadSpeed;
    int m_maxWriteSpeed;
    int m_currentWriteSpeed;


    // only needed for scsi devices
    int m_bus;
    int m_target;
    int m_lun;

    int m_bufferSize;

    int m_writeModes;

  private:
    class Private;
    Private* d;
    friend class DeviceManager;
  };

};

typedef K3bCdDevice::CdDevice K3bDevice;

#endif
