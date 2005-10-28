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

#include <k3bdevicetypes.h>
#include <k3bdiskinfo.h>
#include <k3bcdtext.h>
#include <k3bmsf.h>

#ifdef Q_OS_FREEBSD
struct cam_device;
#endif

namespace K3bDevice
{
  class Toc;

  /**
   * \brief The main class representing a device.
   *
   * Devices are constructed by the DeviceManager.
   */
  class Device
    {
    public:
#ifdef Q_OS_FREEBSD
      typedef struct cam_device* Handle;
#else
    // file descriptor
      typedef int Handle;
#endif

      /**
       * The available cdrdao drivers
       * \deprecated This will be moved to libk3b
       */
      static const char* cdrdao_drivers[];

      // FIXME: make this protected
      ~Device();

      /**
       * The interface type.
       *
       * \return K3bDevice::SCSI or K3bDevice::IDE.
       */
      Interface interfaceType();

      /**
       * The device type.
       *
       * @return A bitwise or of K3bDevice::DeviceType.
       */
      int type() const;

      /**
       * \return Vendor string as reported by the device's firmware.
       */
      const QString& vendor() const { return m_vendor; }

      /**
       * \return Description string as reported by the device's firmware.
       */
      const QString& description() const { return m_description; }

      /**
       * \return Version string as reported by the device's firmware.
       */
      const QString& version() const { return m_version; }

      /**
       * Shortcut for \code writesCd() || writesDvd() \endcode
       *
       * \return true if the device is able to burn media.
       */
      bool burner() const;

      /**
       * Shortcut for \code type() & DEVICE_CD_R \endcode
       *
       * \return true if the device is able to burn CD-R media.
       */
      bool writesCd() const;

      /**
       * Shortcut for \code type() & DEVICE_CD_RW \endcode
       *
       * \return true if the device is able to burn CD-RW media.
       */
      bool writesCdrw() const;

      /**
       * Shortcut for \code writesDvdMinus() || writesDvdPlus() \endcode
       *
       * \return true if the device is able to burn DVD media.
       */
      bool writesDvd() const;


      /**
       * Shortcut for \code type() & (DEVICE_DVD_PLUS_R|DEVICE_DVD_PLUS_RW) \endcode
       *
       * \return true if the device is able to burn DVD+R or DVD+RW media.
       */
      bool writesDvdPlus() const;

      /**
       * Shortcut for \code type() & (DEVICE_DVD_R|DEVICE_DVD_RW) \endcode
       *
       * \return true if the device is able to burn DVD-R or DVD-RW media.
       */
      bool writesDvdMinus() const;

      /**
       * Shortcut for \code type() & DEVICE_DVD_ROM \endcode
       *
       * \return true if the device is able to read DVD media.
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
       * Shortcut for \code writingModes() & WRITINGMODE_SAO \endcode
       *
       * \deprecated use supportsWritingMode()
       */
      bool dao() const;

      /**
       * Check if the device supports a certain writing mode.
       *
       * \return true if the device supports the requested writing mode or false otherwise.
       */
      bool supportsWritingMode( WritingMode mode ) const { return (m_writeModes & mode); }

      /**
       * Shortcut for 
       * \code
       *  writingModes() & (WRITINGMODE_RAW|WRITINGMODE_RAW_R16|WRITINGMODE_RAW_R96P|WRITINGMODE_RAW_R96R)
       * \endcode
       */
      bool supportsRawWriting() const;

      /**
       * @return true if the device is a DVD-R(W) writer which supports test writing.
       */
      bool dvdMinusTestwrite() const { return m_dvdMinusTestwrite; }

      int maxReadSpeed() const { return m_maxReadSpeed; }
      int currentWriteSpeed() const { return m_currentWriteSpeed; }

      /**
       * Size of the device's internal writing buffer.
       *
       * \return The size of the buffer in KB.
       */
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
       * \return All device nodes for this drive.
       */
      const QStringList& deviceNodes() const;

      /**
       * \see K3bDevice::Device::deviceNodes()
       */
      void addDeviceNode( const QString& );

      /**
       * \return The device name used for mounting.
       * \see mountPoint()
       */
      const QString& mountDevice() const;

      /**
       * The mountpoint as found by DeviceManager::scanFstab()
       * \see mountDevice()
       */
      const QString& mountPoint() const;

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

      /**
       * \deprecated the cdrdao driver has no place in this library. It will be removed.
       */
      const QString& cdrdaoDriver() const { return m_cdrdaoDriver; }

      /**
       * @return true if the device is mounted automatically (supermount or subfs)
       */
      bool automount() const { return m_automount; }

      /**
       * returns: 0 auto (no cdrdao-driver selected)
       *          1 yes
       *          2 no
       *
       * \deprecated cdrdao specific stuff has no place in this library. It will be removed.
       */
      int cdTextCapable() const;

      /**
       * internal K3b value.
       * \deprecated This should not be handled here.
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
       * \deprecated the cdrdao driver has no place in this library. It will be removed.
       */
      void setCdrdaoDriver( const QString& d ) { m_cdrdaoDriver = d; }

      /**
       * Only used if the cdrdao-driver is NOT set to "auto".
       * In that case it must be manually set because there
       * is no way to autosense the cd-text capability.
       *
       * \deprecated the cdrdao driver has no place in this library. It will be removed.
       */
      void setCdTextCapability( bool );

      /**
       * checks if unit is ready (medium inserted and ready for command)
       *
       * Refers to the MMC command: TEST UNIT READY
       */
      bool testUnitReady() const;

      /**
       * checks if disk is empty, returns @p K3bDevice::State
       */
      int isEmpty() const;

      /**
       * @return true if inserted media is rewritable.
       */
      bool rewritable() const;

      /**
       * Check if the inserted media is a DVD.
       *
       * \return true if the inserted media is a DVD.
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
       * @return The toc of the media or an empty (invalid) K3bDevice::Toc if 
       *         no or an empty media is inserted.
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
       *
       * \return A CdText object filled with the CD-TEXT values or an empty one in case of
       *         pure data media or if the CD does not contain CD-TEXT.
       */
      CdText readCdText() const;

      /**
       * @return The K3bDevice::Track::DataMode of the track.
       * @see K3bDevice::Track
       */
      int getTrackDataMode( const Track& track ) const;

      /**
       * @return the mode of a data track. K3bDevice::Track::MODE1, K3bDevice::Track::MODE2, 
       *         K3bDevice::Track::XA_FORM1, or K3bDevice::Track::XA_FORM2.
       */
      int getDataMode( const K3b::Msf& sector ) const;

      /**
       * block or unblock the drive's tray
       * \return true on success and false on error.
       * \see eject()
       */
      bool block( bool ) const;

      /**
       * Eject the media.
       * \return true on success and false on error.
       * \see load()
       */
      bool eject() const;

      /**
       * Load the media.
       * @return true on success and false on error.
       */
      bool load() const;

      /**
       * The supported writing modes.
       *
       * \return A bitwise or of K3bDevice::WritingMode or 0 in case of a read-only device.
       */
      int writingModes() const { return m_writeModes; }

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
       * Check if a certain feature is current.
       * \see k3bdevicetypes.h for feature constants.
       * \return 1 if the feature is current, 0 if not, -1 on error
       */
      int featureCurrent( unsigned int feature ) const;

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
       * @return true on success or if the device is already open.
       * @see close()
       */
      bool open( bool write = false ) const;

      /**
       * Close the files descriptor.
       * @see open()
       */
      void close() const;

      /**
       * @return true if the device was successfully opened via @p open()
       */
      bool isOpen() const;

      /**
       * fd on linux, cam on bsd
       */
      Handle handle() const;

      /**
       * \return \li -1 on error (no DVD)
       *         \li 1 (CSS/CPPM)
       *         \li 2 (CPRM) if scrambled
       *         \li 0 otherwise
       */
      int copyrightProtectionSystemType() const;

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

      /**
       * @param pf If false all fields in the descriptor data is vendor specific. Default should be true.
       */
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
       *        \li 00b - value refers to a logical block adress
       *        \li 01b - value refers to a track number where 0 will treat the lead-in as if it
       *                  were a logical track and ffh will read the invisible or incomplete track.
       *        \li 10b - value refers to a session number
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
       * @param sectorType: \li 000b - all types
       *                    \li 001b - CD-DA
       *                    \li 010b - Mode 1
       *                    \li 011b - Mode 2 formless
       *                    \li 100b - Mode 2 form 1
       *                    \li 101b - Mode 2 form 2
       *
       * @param startAdress Lba 0 is mapped to msf 00:00:00 so this method uses
       *                    startAdress+150 as the starting msf.
       *
       * @param endAdress This is the ending adress which is NOT included in the read operation.
       *                  Lba 0 is mapped to msf 00:00:00 so this method uses
       *                  endAdress+150 as the ending msf.
       *
       * @param c2:         \li 00b  - No error info
       *                    \li 01b  - 294 bytes, one bit for every byte of the 2352 bytes
       *                    \li 10b  - 296 bytes, xor of all c2 bits, zero pad bit, 294 c2 bits
       *
       * @param subChannel: \li 000b - No Sub-channel data
       *                    \li 001b - RAW P-W Sub-channel (96 bytes)
       *                    \li 010b - Formatted Q Sub-channel (16 bytes)
       *                    \li 100b - Corrected and de-interleaved R-W Sub-channel (96 bytes)
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
       * @param sectorType: \li 000b - all types
       *                    \li 001b - CD-DA
       *                    \li 010b - Mode 1
       *                    \li 011b - Mode 2 formless
       *                    \li 100b - Mode 2 form 1
       *                    \li 101b - Mode 2 form 2
       *
       * @param c2:         \li 00b  - No error info
       *                    \li 01b  - 294 bytes, one bit for every byte of the 2352 bytes
       *                    \li 10b  - 296 bytes, xor of all c2 bits, zero pad bit, 294 c2 bits
       *
       * @param subChannel: \li 000b - No Sub-channel data
       *                    \li 001b - RAW P-W Sub-channel (96 bytes)
       *                    \li 010b - Formatted Q Sub-channel (16 bytes)
       *                    \li 100b - Corrected and de-interleaved R-W Sub-channel (96 bytes)
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
       * MMC command Read Buffer Capacity
       *
       * \return \see K3bScsiCommand::transport()
       */
      int readBufferCapacity( long long& bufferLength, long long& bufferAvail ) const;

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
      /**
       * A Device can only be constructed the the DeviceManager.
       */
      Device( const QString& devname );

      /**
       * Determines the device's capabilities. This needs to be called once before
       * using the device.
       *
       * Should only be used by the DeviceManager.
       *
       * @param checkWritingModes if true the CD writing modes will be checked using 
       *                          MMC_MODE_SELECT.
       */
      bool init( bool checkWritingModes = true );

      void setMountPoint( const QString& );
      void setMountDevice( const QString& );

      void searchIndexTransitions( long start, long end, K3bDevice::Track& track ) const;
      void checkWritingModes();
      void checkFeatures();
      void checkForJustLink();
      void checkFor2AFeatures();
      void checkForAncientWriters();

      /**
       * Internal method which checks if the raw toc data has bcd values or hex.
       * @return 0 if hex, 1 if bcd, -1 if none
       */
      int rawTocDataWithBcdValues( unsigned char* data, int dataLen ) const;

      bool getSupportedWriteSpeedsVia2A( QValueList<int>& list, bool dvd ) const;
      bool getSupportedWriteSpeedsViaGP( QValueList<int>& list, bool dvd ) const;

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
  int openDevice( const char* name, bool write = false );
#endif
}

#endif
