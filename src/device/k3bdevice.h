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
#include <qvaluelist.h>
#include <qglobal.h>

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

    /**
     * @li CDR: Device can write CDR media.
     * @li CDRW: Device can write CDRW media.
     * @li CDROM: Device can read CD media.
     * @li DVD: Device can read DVD media.
     * @li DVDRAM: Device can write DVDRAM media.
     * @li DVDR: Device can write DVD-R media.
     * @li DVDRW: Device can write DVD-RW media.
     * @li DVDPR: Device can write DVD+R media.
     * @li DVDPRW: Device can write DVD+RW media.
     */
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

    /**
     * The different writing modes.
     */
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
     * A CdDevice should only be constructed the the DeviceManager.
     */
    CdDevice( const QString& devname );
    ~CdDevice();

    /**
     * Determines the device's capabilities. This needs to be called once before
     * using the device.
     *
     * Should only be used by the DeviceManager.
     */
    bool init();

    /**
     * @return The interfact type: SCSI or IDE.
     */
    interface interfaceType();

    /**
     * @return A bitwise OR of the @p DeviceType enumeration.
     */
    int type() const;

    const QString& vendor() const { return m_vendor; }
    const QString& description() const { return m_description; }
    const QString& version() const { return m_version; }

    /**
     * @return Equal to writesCd() || writesDvd()
     */
    bool burner() const;

    /**
     * Shortcut for (type() & CDR)
     */
    bool writesCd() const;

    /**
     * Shortcut for (type() & CDRW)
     */
    bool writesCdrw() const;

    /**
     * @return Equal to writesDvdMinus() || writesDvdPlus()
     */
    bool writesDvd() const;


    /**
     * Shortcut for (type() & DVDPR|DVDPRW)
     */
    bool writesDvdPlus() const;

    /**
     * Shortcut for (type() & DVDR|DVDRW)
     */
    bool writesDvdMinus() const;

    /**
     * Shortcut for (type() & DVD)
     */
    bool readsDvd() const;

    /**
     * @deprecated Use burnfree()
     */
    bool burnproof() const;

    /**
     * @return true is the device is a writer and supports buffer underrun free recording (BURNFREE)
     */
    bool burnfree() const;

    /**
     * Shortcut for (writingModes() & DAO)
     */
    bool dao() const;

    /**
     * @return true if the device is a DVD-R(W) writer which supports test writing.
     */
    bool dvdMinusTestwrite() const { return m_dvdMinusTestwrite; }

    int maxReadSpeed() const { return m_maxReadSpeed; }
    int currentWriteSpeed() const { return m_currentWriteSpeed; }

    int bufferSize() const { return m_bufferSize; }

    /**
     * @return the corresponding device name.
     */
    const QString& devicename() const;

    /**
     * for SCSI devices this should be something like /dev/scd0 or /dev/sr0
     * for IDE device this should be something like /dev/hdb1
     */
    const QString& blockDeviceName() const;

    /**
     * @return All device nodes for this drive.
     * This mainly makes sense with scsi devices which
     * can be accessed through /dev/scdX or /dev/srX
     * and is useful for fstab scanning.
     */
    const QStringList& deviceNodes() const;

    /**
     * @see K3bCdDevice::CdDevice::deviceNodes()
     */
    void addDeviceNode( const QString& );

    /**
     * @return The device name used for mounting.
     * @see K3bCdDevice::CdDevice::deviceNodes()
     */
    const QString& mountDevice() const;

    /** 
     * Makes only sense to use with scsi devices 
     * @return a string for use with the cdrtools
     * @deprecated
     */
    QString busTargetLun() const;

    int scsiBus() const { return m_bus; }
    int scsiId() const { return m_target; }
    int scsiLun() const { return m_lun; }

    int maxWriteSpeed() const { return m_maxWriteSpeed; }
    const QString& cdrdaoDriver() const { return m_cdrdaoDriver; }

    const QString& mountPoint() const;

    /**
     * @return true if the device is mounted via supermount
     */
    bool supermount() const { return m_supermount; }

    /**
     * returns: 0 auto (no cdrdao-driver selected)
     *          1 yes
     *          2 no
     */
    int cdTextCapable() const;


    /** 
     * internally K3b value.
     */
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

    void setMountPoint( const QString& );
    void setMountDevice( const QString& d );


    /** 
     * checks if unit is ready (medium inserted and ready for command)
     *
     * Refers to the MMC command: TEST UNIT READY
     */
    bool isReady() const;

    /**
     * checks if disk is empty, returns @p DiskStatus
     */
    int isEmpty() const;

    /**
     * @return true if inserted media is rewritable.
     */
    bool rewritable() const;

    /**
     * @return true if the inserted media is a DVD.
     */
    bool isDVD() const;

    /**
     * returnes the complete diskinfo. This includes the toc.
     * @deprecated use @p ngDiskInfo()
     */
    DiskInfo diskInfo();

    /**
     * @return The number of sessions on the media.
     */
    int numSessions() const;

    /**
     * @return The media size.
     */
    K3b::Msf discSize() const;

    /**
     * @return The remaining size on an appendable media
     */
    K3b::Msf remainingSize() const;

    /**
     * @return The toc of the media
     */
    Toc readToc() const;

    /**
     * Read the CD-TEXT of an audio or mixed-mode CD.
     * @param trackCount if specified this method doed not need to determine them which saves time.
     */
    AlbumCdText readCdText( unsigned int trackCount = 0 ) const;
    
    /**
     * @return The DataMode of the track
     * @see K3bCdDevice::Track
     */
    int getTrackDataMode( int track ) const;

    /**
     * block or unblock the drive's tray
     * @return true on success and false on error.
     */
    bool block( bool ) const;

    /**
     * Eject the media.
     * @return true on success and false on error.
     */
    bool eject() const;

    /**
     * Load the media.
     * @return true on success and false on error.
     */
    bool load() const;

    /**
     * @return The supported writing modes (bitwise OR of @p WriteMode) if the device is a writer.
     */
    int writingModes() const { return m_writeModes; }

    /**
     * Shortcut for (writingModes() & mode)
     */
    bool supportsWriteMode( WriteMode mode );

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
     * @returns true if the requested feature is supported with the mounted media.
     */
    bool supportsFeature( unsigned int feature ) const;

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
     * Does only make sense for cd media.
     * @returns -1 on error K3bCdDevice::MediaType otherwise
     */
    int cdMediaType() const;

    /**
     * Does only make sense for dvd media.
     * @returns -1 on error K3bCdDevice::MediaType otherwise
     */
    int dvdMediaType() const;

    /**
     * Returnes the list of supported writing speeds as reported by 
     * mode page 2Ah.
     *
     * This only works with MMC3 compliant drives.
     */
    QValueList<int> determineSupportedWriteSpeeds() const;

    /**
     * @returnes the speed in kb/s or 0 on failure.
     */
    int determineMaximalWriteSpeed() const;

    /**
     * Open the device for acces via a file descriptor.
     * @return an open file descriptor on success or -1 on failure
     * @see close()
     */
    int open() const;

    /**
     * Close the files descriptor.
     * @see open()
     */
    void close() const;

    /**
     * @return true if the device was successfully opened via @p open()
     */
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
     * @param type specifies what value means:
     *             00b - value refers to a logical block adress
     *             01b - value refers to a track number where 0 will treat the lead-in as if it 
     *                   were a logical track and ffh will read the invisible or incomplete track.
     *             10b - value refers to a session number
     *             
     */
    bool readTrackInformation( unsigned char** data, int& dataLen, int type, unsigned long value ) const;

    /**
     * if true is returned dataLen specifies the actual length of *data which needs to be
     * deleted after using.
     */
    bool mechanismStatus( unsigned char** data, int& dataLen ) const;

    /**
     * Read a single feature.
     * data will be filled with the feature header and the descriptor
     */
    bool getFeature( unsigned char** data, int& dataLen, unsigned int feature ) const;

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
    bool indexScan( K3bCdDevice::Toc& toc ) const;

    /**
     * Seek to the specified sector.
     */
    bool seek( unsigned long lba ) const;

  protected:
    bool furtherInit();

#ifdef Q_OS_LINUX
    /**
     * Fallback method that uses the evil cdrom.h stuff
     */
    bool readTocLinux( Toc& ) const;
#endif

    /**
     * The preferred toc reading method for all CDs. Also reads session info.
     * undefined for DVDs.
     */
    bool readRawToc( Toc& ) const;
    bool readFormattedToc( Toc&, bool dvd = false ) const;

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
    QString m_cdrdaoDriver;
    int m_cdTextCapable;
    int m_maxReadSpeed;
    int m_maxWriteSpeed;
    int m_currentWriteSpeed;

    bool m_dvdMinusTestwrite;

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
   *
   * @internal
   */
  int openDevice( const char* name );
}

typedef K3bCdDevice::CdDevice K3bDevice;

#endif
