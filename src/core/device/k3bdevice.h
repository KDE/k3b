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
#include <k3bdiskinfo.h>
#include <k3bcdtext.h>
#include <k3bmsf.h>


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
                         DVD = 8,
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
		     RAW = 4,
                     PACKET = 8,
                     SAO_R96P = 16,
                     SAO_R96R = 32,
                     RAW_R16 = 64,
                     RAW_R96P = 128,
                     RAW_R96R = 256 };


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

    // depr. use writesCd
    bool           burner() const;
    bool           writesCd() const;
    bool           writesCdrw() const;
    bool           writesDvd() const;
    bool           readsDvd() const;
    bool           burnproof() const;
    bool           burnfree() const;
    bool           dao() const;
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

    bool supermount() const { return m_supermount; }

    /**
     * returns: 0 auto (no cdrdao-driver selected)
     *          1 yes
     *          2 no
     */
    int cdTextCapable() const;


    /** internally K3b value. */
    void setCurrentWriteSpeed( int s ) { m_currentWriteSpeed = s; }

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
    void setBufferSize( int b ) { m_bufferSize = b; }

    void setMountPoint( const QString& );
    void setMountDevice( const QString& d );

    /** 
     * checks if unit is ready (medium inserted and ready for command)
     */
    bool isReady() const;


    /**
     *  checks if disk is empty, returns DiskStatus:
     */
    int isEmpty() const;

    /**
     *  checks if disk is rewritable
     */
    bool rewritable() const;

    /**
     *  try to read physical DVD structure
     */
    bool isDVD() const;

    /**
     * returnes the complete diskinfo. This includes the toc.
     */
    DiskInfo diskInfo();

    /**
    *  returnes the discType from the Full-TOC mmc-command.
    */
    int tocType() const;

    /**
     *  returns the number of sessions on disk
     */
    int numSessions() const;

    /**
     *  returns the disc size
     */
    K3b::Msf discSize() const;

    /**
     *  returns the remaining disc size
     */
    K3b::Msf remainingSize() const;

    /**
     *  returns the toc of the disc
     */
    Toc readToc() const;

    /**
     * read the CD-TEXT. 
     * @param trackCount if specified this method doed not need to determine them which saves time.
     */
    AlbumCdText readCdText( unsigned int trackCount = 0 ) const;

     /**
     *  returns the DataMode of the track
     */
    int getTrackDataMode(int track) const;

     /**
     *  returns the DataMode of the track
     */
    //    int getTrackHeader(int lba);

    /**
     * block or unblock the drive's tray
     * returns true on success and false on scsi-error
     */
    bool block( bool ) const;

    bool eject() const;
    bool load() const;

    int writingModes() const { return m_writeModes; }
    bool supportsWriteMode( WriteMode );

    bool readSectorsRaw(unsigned char *buf, int start, int count) const;

    /**
     * Get a list of supported profiles. See enumeration MediaType.
     */
    int supportedProfiles() const;

    /**
     * Tries to get the current profile from the drive.
     * @returns -1 on error (command failed or unknown profile)
     *          MediaType otherwise (MEDIA_NONE means: no current profile)
     */
    int currentProfile() const;

    /**
     * This is the method to use!
     */
    NextGenerationDiskInfo ngDiskInfo() const;

    /**
     * Refers to MMC command READ CAPACITY
     */
    bool readCapacity( K3b::Msf& ) const;

    /**
     * Refers to MMC command READ FORMAT CAPACITY
     */
    bool readFormatCapacity( K3b::Msf& ) const;

    /**
     * Does only make sense for dvd media.
     * @returns -1 on error K3bCdDevice::MediaType otherwise
     */
    int dvdMediaType() const;

    /**
     * @returnes the speed in kb/s or 0 on failure.
     */
    int determineOptimalWriteSpeed() const;

    /**
     * @return fd on success; -1 on failure
     */
    int open() const;
    void close() const;
    bool isOpen() const;


    // MMC commands

    /**
     * if true is returned dataLen specifies the actual length of *data which needs to be
     * deleted after using.
     */
    bool readDiscInfo( unsigned char** data, int& dataLen ) const;

    bool modeSelect( unsigned char* page, int pageLen, bool pf, bool sp ) const;

    /**
     * if true is returned pageLen specifies the actual length of *pageData which needs to be
     * deleted after using.
     */
    bool modeSense( unsigned char** pageData, int& pageLen, int page ) const;

    /**
     * if true is returned dataLen specifies the actual length of *data which needs to be
     * deleted after using.
     */
    bool readTocPmaAtip( unsigned char** data, int& dataLen, int format, bool time, int track ) const;

    /**
     * if true is returned dataLen specifies the actual length of *data which needs to be
     * deleted after using.
     */
    bool mechanismStatus( unsigned char** data, int& dataLen ) const;

    /**
     * @param sectorType: 000b - all types
     *                    001b - CD-DA
     *                    010b - Mode 1
     *                    011b - Mode 2 formless
     *                    100b - Mode 2 form 1
     *                    101b - Mode 2 form 2
     *
     * @param c2:         00b  - No error info
     *                    01b  - 294 bytes, one bit for every byte of the 2352 bytes
     *                    10b  - 296 bytes, xor of all c2 bits, zero pad bit, 294 c2 bits
     *
     * @param subChannel: 000b - No Sub-channel data
     *                    001b - RAW P-W Sub-channel (96 bytes)
     *                    010b - Formatted Q Sub-channel (16 bytes)
     *                    100b - Corrected and de-interleaved R-W Sub-channel (96 bytes)
     */
    bool readCd( unsigned char* data, 
		 int dataLen,
		 int sectorType,
		 bool dap,
		 unsigned long startAdress,
		 unsigned long length,
		 bool sync,
		 bool header,
		 bool subHeader,
		 bool userData,				    
		 bool edcEcc,
		 int c2,
		 int subChannel ) const;

    bool read10( unsigned char* data,
		 int dataLen,
		 unsigned long startAdress,
		 unsigned int length,
		 bool fua = false ) const;

    bool read12( unsigned char* data,
		 int dataLen,
		 unsigned long startAdress,
		 unsigned long length,
		 bool streaming = false,
		 bool fua = false ) const;

    /**
     * @param subchannelParam: 01h - CD current position
     *                         02h - Media Catalog number (UPC/bar code)
     *                         03h - ISRC
     * @param trackNumber only valid if subchannelParam == 03h
     */
    bool readSubChannel( unsigned char** data, 
			 int& dataLen,
			 unsigned int subchannelParam,
			 unsigned int trackNumber ) const;

    /**
     * @returns the index number on success
     *          -1 on general error
     *          and -2 if there is no index info in that frame
     */
    int getIndex( unsigned long lba ) const;

    bool searchIndex0( unsigned long startSec, unsigned long endSec, long& pregapStart ) const;

    /**
     * For now this just searches index 0 for all tracks and sets
     * the value in the tracks.
     * In the future this should scan for all indices.
     */
    void indexScan( K3bCdDevice::Toc& toc ) const;

    /**
     * Seek to the specified sector.
     */
    bool seek( unsigned long lba ) const;

  protected:
    bool furtherInit();

    /**
     * Fallback method that uses the evil cdrom.h stuff
     */
    bool readTocLinux( Toc& ) const;

    /**
     * The preferred toc reading method for all CDs. Also reads session info.
     * undefined for DVDs.
     */
    bool readRawToc( Toc& ) const;

    /**
     * Fixes the last block on CD-Extra disks. This is needed if the readRawToc failed since
     * in that case the first sector of the last session's first track is used as the previous
     * session's last track's last sector which is wrong. There is a 11400 block session lead-in
     * between them. This method fixes this only for the last session and only on linux.
     */
    bool fixupToc( Toc& ) const;

    void checkWriteModes();

    QString m_vendor;
    QString m_description;
    QString m_version;
    bool m_burner;
    bool m_bWritesCdrw;
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

    bool m_supermount;

  private:
    class Private;
    Private* d;
    friend class DeviceManager;
  };

  /**
   * This should always be used to open a device since it
   * uses the resmgr
   */
  int openDevice( const char* name );
}

typedef K3bCdDevice::CdDevice K3bDevice;

#endif
