/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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

#ifdef Q_OS_FREEBSD
struct cam_device;
#endif

namespace K3bDevice
{
  const unsigned char FEATURE_PROFILE_LIST = 0x000;
  const unsigned char FEATURE_CORE = 0x001;
  const unsigned char FEATURE_MORPHING = 0x002;
  const unsigned char FEATURE_REMOVABLE_MEDIA = 0x003;
  const unsigned char FEATURE_WRITE_PROTECT = 0x004;
  const unsigned char FEATURE_RANDOM_READABLE = 0x010;
  const unsigned char FEATURE_MULTI_READ = 0x01D;
  const unsigned char FEATURE_CD_READ = 0x01E;
  const unsigned char FEATURE_DVD_READ = 0x01F;
  const unsigned char FEATURE_RANDOM_WRITABLE = 0x020;
  const unsigned char FEATURE_INCREMENTAL_STREAMING_WRITABLE = 0x021;
  const unsigned char FEATURE_SECTOR_ERASABLE = 0x022;
  const unsigned char FEATURE_FORMATTABLE = 0x023;
  const unsigned char FEATURE_DEFECT_MANAGEMENT = 0x024;
  const unsigned char FEATURE_WRITE_ONCE = 0x025;
  const unsigned char FEATURE_RESTRICTED_OVERWRITE = 0x026;
  const unsigned char FEATURE_CD_RW_CAV_WRITE = 0x027;
  const unsigned char FEATURE_MRW = 0x028;
  const unsigned char FEATURE_ENHANCED_DEFECT_REPORTING = 0x029;
  const unsigned char FEATURE_DVD_PLUS_RW = 0x02A;
  const unsigned char FEATURE_DVD_PLUS_R = 0x02B;
  const unsigned char FEATURE_RIGID_RESTRICTED_OVERWRITE = 0x02C;
  const unsigned char FEATURE_CD_TRACK_AT_ONCE = 0x02D;
  const unsigned char FEATURE_CD_MASTERING = 0x02E;
  const unsigned char FEATURE_DVD_R_RW_WRITE = 0x02F;
  const unsigned char FEATURE_DDCD_READ = 0x030;
  const unsigned char FEATURE_DDCD_R_WRITE = 0x031;
  const unsigned char FEATURE_DDCD_RW_WRITE = 0x032;
  const unsigned char FEATURE_CD_RW_MEDIA_WRITE_SUPPORT = 0x37;
  /*
  FIXME:
../../libk3bdevice/k3bdevice.h:63: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:64: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:65: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:66: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:67: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:68: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:69: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:70: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:71: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:72: warning: large integer implicitly truncated to unsigned type
../../libk3bdevice/k3bdevice.h:73: warning: large integer implicitly truncated to unsigned type
   */
  const unsigned char FEATURE_POER_MANAGEMENT = 0x100;
  const unsigned char FEATURE_EMBEDDED_CHANGER = 0x102;
  const unsigned char FEATURE_CD_AUDIO_ANALOG_PLAY = 0x103;
  const unsigned char FEATURE_MICROCODE_UPGRADE = 0x104;
  const unsigned char FEATURE_TIMEOUT = 0x105;
  const unsigned char FEATURE_DVD_CSS = 0x106;
  const unsigned char FEATURE_REAL_TIME_STREAMING = 0x107;
  const unsigned char FEATURE_LOGICAL_UNIT_SERIAL_NUMBER = 0x108;
  const unsigned char FEATURE_DISC_CONTROL_BLOCKS = 0x10A;
  const unsigned char FEATURE_DVD_CPRM = 0x10B;
  const unsigned char FEATURE_FIRMWARE_DATE = 0x10C;


  class Toc;

  class Device
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
     * A Device should only be constructed the the DeviceManager.
     */
    Device( const QString& devname );
    ~Device();

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
     * Shortcut for (writingModes() & (RAW|RAW_R16|RAW_R96P|RAW_R96R))
     */
    bool supportsRawWriting() const;

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
    const QString& blockDeviceName() const { return m_blockDevice; }

    /**
     * This is only valid for SCSI devices. Without devfs it's something
     * like /dev/sg0. Otherwise something like /dev/scsi/host0/bus0/target0/lun0/generic.
     *
     * This is not needed in K3b at all. But cdrecord and cdrdao use the sg devices and
     * we need it to fixup it's permissions in K3bSetup.
     */
    const QString& genericDevice() const { return m_genericDevice; }

    /**
     * @return All device nodes for this drive.
     * This mainly makes sense with scsi devices which
     * can be accessed through /dev/scdX or /dev/srX
     * and is useful for fstab scanning.
     */
    const QStringList& deviceNodes() const;

    /**
     * @see K3bDevice::Device::deviceNodes()
     */
    void addDeviceNode( const QString& );

    /**
     * @return The device name used for mounting.
     * @see K3bDevice::Device::deviceNodes()
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
     * @return true if the device is mounted automatically (supermount or subfs)
     */
    bool automount() const { return m_automount; }

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
     * Append ISRC and MCN to the TOC if found
     * This has been moved to a seperate method since it can take a very long time
     * to scan for all ISRCs.
     */
    void readIsrcMcn( Toc& toc ) const;

    /**
     * Read the CD-TEXT of an audio or mixed-mode CD.
     */
    CdText readCdText() const;

    /**
     * @return The DataMode of the track
     * @see K3bDevice::Track
     */
    int getTrackDataMode( const Track& track ) const;

    /**
     * @return the mode of a data track. mode1, mode2 form/1/2 or formless
     */
    int getDataMode( const K3b::Msf& sector ) const;

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
     * @returns true if the requested feature is supported and also current.
     * @deprecated use featureCurrent
     */
    bool supportsFeature( unsigned int feature ) const;

    bool featureCurrent( unsigned int feature ) const;

    /**
     * This is the method to use!
     */
    DiskInfo diskInfo() const;

    /**
     * Refers to MMC command READ CAPACITY
     */
    bool readCapacity( K3b::Msf& ) const;

    /**
     * Refers to MMC command READ FORMAT CAPACITY
     *
     * @param wantedFormat The requested format type.
     * @param result If true is returned this contains the requested value.
     * @param currentMax If not 0 this will be filled with the Current/Maximum Descriptor value.
     * @param currentMax If not 0 this will be filled with the Current/Maximum Format Type.
     */
    bool readFormatCapacity( int wantedFormat, K3b::Msf& result,
			     K3b::Msf* currentMax = 0, int* currentMaxFormat = 0 ) const;

    /**
     * Does only make sense for cd media.
     * @returns -1 on error K3bDevice::MediaType otherwise
     */
    int cdMediaType() const;

    /**
     * Does only make sense for dvd media.
     * @returns -1 on error K3bDevice::MediaType otherwise
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
     * SET SPEED command
     *
     * @param readingSpeed The preferred reading speed (0x0000-0xFFFE). 0xFFFF requests
     *                     fot the logical unit to select the optimal speed.
     * @param writingSpeed The preferred writing speed (0x0000-0xFFFE). 0xFFFF requests
     *                     fot the logical unit to select the optimal speed.
     * @param cav Is the speed pure CAV?
     */
    bool setSpeed( unsigned int readingSpeed,
		   unsigned int writingSpeed,
		   bool cav = false ) const;

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
    bool readDvdStructure( unsigned char** data, int& dataLen, 
			   unsigned int format = 0x0,
			   unsigned int layer = 0x0,
			   unsigned long adress = 0,
			   unsigned int agid = 0x0 ) const;

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
     * if true is returned dataLen specifies the actual length of *data which needs to be
     * deleted after using.
     */
    bool getPerformance( unsigned char** data, int& dataLen,
			 unsigned int type,
			 unsigned int dataType,
			 unsigned int lba = 0 ) const;

    /**
     * @param sectorType: 000b - all types
     *                    001b - CD-DA
     *                    010b - Mode 1
     *                    011b - Mode 2 formless
     *                    100b - Mode 2 form 1
     *                    101b - Mode 2 form 2
     *
     * @param startAdress K3b does map lba 0 to msf 00:00:00 so this method uses
     *                    startAdress+150 as the starting msf.
     *
     * @param endAdress This is the ending adress which is NOT included in the read operation.
     *                  K3b does map lba 0 to msf 00:00:00 so this method uses
     *                  endAdress+150 as the ending msf.
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
    bool readCdMsf( unsigned char* data,
		    int dataLen,
		    int sectorType,
		    bool dap,
		    const K3b::Msf& startAdress,
		    const K3b::Msf& endAdress,
		    bool sync,
		    bool header,
		    bool subHeader,
		    bool userData,
		    bool edcEcc,
		    int c2,
		    int subChannel ) const;

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

    bool readIsrc( unsigned int track, QCString& isrc ) const;

    bool readMcn( QCString& mcn ) const;

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
    bool indexScan( K3bDevice::Toc& toc ) const;

    /**
     * Seek to the specified sector.
     */
    bool seek( unsigned long lba ) const;

#ifdef Q_OS_FREEBSD
    /**
     * Return the SCSI control handle (only FBSD)
     */
    struct cam_device *cam() const;
#endif

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

  private:
    void searchIndexTransitions( long start, long end, K3bDevice::Track& track ) const;
    void checkWriteModes();
    void checkForAncientWriters();

    /**
     * Internal method which checks if the raw toc data has bcd values or hex.
     * @return 0 if hex, 1 if bcd, -1 if none
     */
    int rawTocDataWithBcdValues( unsigned char* data, int dataLen ) const;

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

    bool m_automount;

    // only needed on FreeBSD
    QString m_passDevice;
    QString m_blockDevice;
    QString m_genericDevice;

    class Private;
    Private* d;
    friend class DeviceManager;
  };

#ifdef Q_OS_LINUX
  /**
   * This should always be used to open a device since it
   * uses the resmgr
   *
   * @internal
   */
  int openDevice( const char* name );
#endif
}

#endif
