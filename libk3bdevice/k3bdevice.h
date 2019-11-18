/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDEVICE_H
#define K3BDEVICE_H

#include "k3bdevicetypes.h"
#include "k3bdiskinfo.h"
#include "k3bcdtext.h"
#include "k3bmsf.h"
#include "k3bdevice_export.h"

#include <qglobal.h>
#include <QVarLengthArray>

#if defined(__FreeBSD_kernel__)
#undef Q_OS_LINUX
#define Q_OS_FREEBSD 1
#endif

#ifdef Q_OS_FREEBSD
struct cam_device;
#endif

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

namespace Solid {
    class Device;
    class StorageAccess;
}

namespace K3b {
    namespace Device
    {
        class Toc;

        typedef QVarLengthArray< unsigned char > UByteArray;

        /**
         * \brief The main class representing a device.
         *
         * Devices are constructed by the DeviceManager.
         *
         * All methods except for open and close in Device are thread-safe which basically means that
         * no two commands are sent to the device at the same time.
         */
        // FIXME: all methods are const which makes no sense at all!
        class LIBK3BDEVICE_EXPORT Device
        {
        public:
#if defined(Q_OS_FREEBSD)
            typedef struct cam_device* Handle;
#elif defined(Q_OS_WIN32)
            // file handle
            typedef HANDLE Handle;
#else
            // file descriptor
            typedef int Handle;
#endif

            ~Device();

            Solid::Device solidDevice() const;

            /**
             *  Gives interface for dics volume.
             *  Keep in mind that it can return empty pointer when no disc is inserted.
             *
             *  @return Interface for disc volume.
             */
            Solid::StorageAccess* solidStorage() const;

            /**
             * \deprecated use readCapabilities() and writeCapabilities()
             * The device type.
             *
             * @return A bitwise or of K3b::Device::DeviceType.
             */
            DeviceTypes type() const;

            /**
             * The mediatypes this device is able to read.
             *
             * \return A bitwise or of K3b::Device::MediaType
             */
            MediaTypes readCapabilities() const;

            /**
             * The media types this device is able to write.
             *
             * \return A bitwise or of K3b::Device::MediaType
             */
            MediaTypes writeCapabilities() const;

            /**
             * \return Vendor string as reported by the device's firmware.
             */
            QString vendor() const;

            /**
             * \return Description string as reported by the device's firmware.
             */
            QString description() const;

            /**
             * \return Version string as reported by the device's firmware.
             */
            QString version() const;

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
            bool supportsWritingMode( WritingMode mode ) const { return (writingModes() & mode); }

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
            bool dvdMinusTestwrite() const;

            int maxReadSpeed() const;
            int currentWriteSpeed() const;

            /**
             * Size of the device's internal writing buffer.
             *
             * \return The size of the buffer in KB.
             */
            int bufferSize() const;

            /**
             * for SCSI devices this should be something like /dev/scd0 or /dev/sr0
             * for IDE device this should be something like /dev/hdb1
             */
            QString blockDeviceName() const;

            int maxWriteSpeed() const;

            /**
             * internal K3b value.
             * \deprecated This should not be handled here.
             */
            void setCurrentWriteSpeed( int s );

            /**
             * Use this if the speed was not detected correctly.
             */
            void setMaxReadSpeed( int s );

            /**
             * Use this if the speed was not detected correctly.
             */
            void setMaxWriteSpeed( int s );

            /**
             * checks if unit is ready (medium inserted and ready for command)
             *
             * Refers to the MMC command: TEST UNIT READY
             */
            bool testUnitReady() const;

            /**
             * checks if disk is empty, returns @p K3b::Device::State
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
             * @return The toc of the media or an empty (invalid) K3b::Device::Toc if
             *         no or an empty media is inserted.
             */
            Toc readToc() const;

            /**
             * Append ISRC and MCN to the TOC if found
             * This has been moved to a separate method since it can take a very long time
             * to scan for all ISRCs.
             */
            void readIsrcMcn( Toc& toc ) const;

            /**
             * Read the raw CD-Text data without decoding it.
             * \return An array of bytes as read from the device, suitable to be used in
             * CdText( const QByteArray& )
             *
             * \sa readCdText
             */
            QByteArray readRawCdText( bool* success = 0 ) const;

            /**
             * Read the CD-TEXT of an audio or mixed-mode CD.
             *
             * \return A CdText object filled with the CD-TEXT values or an empty one in case of
             *         pure data media or if the CD does not contain CD-TEXT.
             *
             * \sa readRawCdText
             */
            CdText readCdText() const;

            /**
             * @return The K3b::Device::Track::DataMode of the track.
             * @see K3b::Device::Track
             */
            Track::DataMode getTrackDataMode( const Track& track ) const;

            /**
             * @return the mode of a data track. K3b::Device::Track::MODE1, K3b::Device::Track::MODE2,
             *         K3b::Device::Track::XA_FORM1, or K3b::Device::Track::XA_FORM2.
             */
            Track::DataMode getDataMode( const K3b::Msf& sector ) const;

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
             * Enable or disable auto-ejecting. For now this is a no-op on non-Linux systems.
             * \param enabled if true auto-ejecting will be enabled, otherwise disabled.
             * \return true if the operation was successful, false otherwise
             */
            bool setAutoEjectEnabled( bool enabled ) const;

            /**
             * The supported writing modes.
             *
             * \return A bitwise or of K3b::Device::WritingMode or 0 in case of a read-only device.
             */
            WritingModes writingModes() const;

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
             * Determine the type of the currently mounted medium
             *
             * @returns K3b::Device::MediaType
             */
            MediaType mediaType() const;

            /**
             * Returns the list of supported writing speeds as reported by
             * mode page 2Ah.
             *
             * This only works with MMC3 compliant drives.
             */
            QList<int> determineSupportedWriteSpeeds() const;

            /**
             * @returns the speed in kb/s or 0 on failure.
             */
            int determineMaximalWriteSpeed() const;

            /**
             * Open the device for access via a file descriptor.
             * @return true on success or if the device is already open.
             * @see close()
             *
             * Be aware that this method is not thread-safe.
             */
            bool open( bool write = false ) const;

            /**
             * Close the files descriptor.
             * @see open()
             *
             * Be aware that this method is not thread-safe.
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
             * if true is returned \param data is resized.
             */
            bool readDiscInformation( UByteArray& data ) const;

            /**
             * @param pf If false all fields in the descriptor data is vendor specific. Default should be true.
             */
            bool modeSelect( UByteArray& pageData, bool pf, bool sp ) const;

            /**
             * if true is returned \param pageData is resized.
             */
            bool modeSense( UByteArray& pageData, int page ) const;

            /**
             * if true is returned \param data is resized.
             */
            bool readTocPmaAtip( UByteArray& data, int format, bool msf, int track ) const;

            /**
             * @param type specifies what value means:
             *        \li 00b - value refers to a logical block address
             *        \li 01b - value refers to a track number where 0 will treat the lead-in as if it
             *                  were a logical track and ffh will read the invisible or incomplete track.
             *        \li 10b - value refers to a session number
             *
             */
            bool readTrackInformation( UByteArray& data, int type, int value ) const;

            /**
             * if true is returned \param data is resized.
             */
            bool readDiscStructure( UByteArray& data,
                                    unsigned int mediaType = 0x0,
                                    unsigned int format = 0x0,
                                    unsigned int layer = 0x0,
                                    unsigned long address = 0,
                                    unsigned int agid = 0x0 ) const;

            /**
             * In MMC5 readDvdStructure was renamed to readDiscStructure. This method does the same
             * like the above.
             */
            bool readDvdStructure( UByteArray& data,
                                   unsigned int format = 0x0,
                                   unsigned int layer = 0x0,
                                   unsigned long address = 0,
                                   unsigned int agid = 0x0 ) const;

            /**
             * if true is returned \param data is resized.
             */
            bool mechanismStatus( UByteArray& data ) const;

            /**
             * Read a single feature.
             * data will be filled with the feature header and the descriptor
             */
            bool getFeature( UByteArray& data, unsigned int feature ) const;


            /**
             * if true is returned \param data is resized.
             */
            bool getPerformance( UByteArray& data,
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
             * @param endAdress This is the ending address which is NOT included in the read operation.
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
                            unsigned int dataLen,
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
                         unsigned int dataLen,
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
                         unsigned int dataLen,
                         unsigned long startAdress,
                         unsigned int length,
                         bool fua = false ) const;

            bool read12( unsigned char* data,
                         unsigned int dataLen,
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
            bool readSubChannel( UByteArray& data,
                                 unsigned int subchannelParam,
                                 unsigned int trackNumber ) const;

            bool readIsrc( unsigned int track, QByteArray& isrc ) const;

            bool readMcn( QByteArray& mcn ) const;

            /**
             * MMC command Read Buffer Capacity
             *
             * \return \see ScsiCommand::transport()
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
            bool indexScan( Toc& toc ) const;

            /**
             * Seek to the specified sector.
             */
            bool seek( unsigned long lba ) const;

            bool getNextWritableAdress( unsigned int& lastSessionStart, unsigned int& nextWritableAdress ) const;

            /**
             * Retrieve the next writable address from the currently mounted writable medium.
             * \return The next writable address if the medium is empty or appendable or -1
             * if an error occurred.
             */
            int nextWritableAddress() const;

            /**
             * Locks the device for usage. This means that no MMC command can be performed
             * until usageUnlock is called.
             *
             * Locking a device is useful when an external application or library is called
             * that opens the device itself.
             *
             * \sa usageUnlock
             */
            void usageLock() const;

            /**
             * Unlock the device after a call to usageLock.
             */
            void usageUnlock() const;

            /**
             * Thread-safe ioctl call for this device for Linux and Net-BSD systems.
             * Be aware that so far this does not include opening the device
             */
//      int ioctl( int request, ... ) const;

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
            bool readFormattedToc( Toc&, int mediaType ) const;

            /**
             * Fixes the last block on CD-Extra disks. This is needed if the readRawToc failed since
             * in that case the first sector of the last session's first track is used as the previous
             * session's last track's last sector which is wrong. There is a 11400 block session lead-in
             * between them. This method fixes this only for the last session and only on linux.
             */
            bool fixupToc( Toc& ) const;

        private:
            /**
             * A Device can only be constructed by the DeviceManager.
             */
            Device( const Solid::Device& dev );

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

            void searchIndexTransitions( long start, long end, K3b::Device::Track& track ) const;
            void checkWritingModes();
            void checkFeatures();
            void checkForJustLink();
            void checkFor2AFeatures();
            void checkForAncientWriters();

            /**
             * Internal method which checks if the raw toc data has bcd values or hex.
             * @return 0 if hex, 1 if bcd, -1 if none
             */
            int rawTocDataWithBcdValues( const UByteArray& data ) const;

            bool getSupportedWriteSpeedsVia2A( QList<int>& list, MediaType type ) const;
            bool getSupportedWriteSpeedsViaGP( QList<int>& list, MediaType type ) const;

            int getMaxWriteSpeedVia2A() const;

            QByteArray mediaId( int mediaType ) const;

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
        K3b::Device::Device::Handle openDevice( const char* name, bool write = false );
    }
}

#endif
