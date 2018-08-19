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


#ifndef _K3B_DEVICE_TYPES_H_
#define _K3B_DEVICE_TYPES_H_

#include <QFlags>

namespace K3b {
    namespace Device {
        const unsigned short FEATURE_PROFILE_LIST = 0x000;
        const unsigned short FEATURE_CORE = 0x001;
        const unsigned short FEATURE_MORPHING = 0x002;
        const unsigned short FEATURE_REMOVABLE_MEDIA = 0x003;
        const unsigned short FEATURE_WRITE_PROTECT = 0x004;
        const unsigned short FEATURE_RANDOM_READABLE = 0x010;
        const unsigned short FEATURE_MULTI_READ = 0x01D;
        const unsigned short FEATURE_CD_READ = 0x01E;
        const unsigned short FEATURE_DVD_READ = 0x01F;
        const unsigned short FEATURE_RANDOM_WRITABLE = 0x020;
        const unsigned short FEATURE_INCREMENTAL_STREAMING_WRITABLE = 0x021;
        const unsigned short FEATURE_SECTOR_ERASABLE = 0x022;
        const unsigned short FEATURE_FORMATTABLE = 0x023;
        const unsigned short FEATURE_DEFECT_MANAGEMENT = 0x024;
        const unsigned short FEATURE_WRITE_ONCE = 0x025;
        const unsigned short FEATURE_RESTRICTED_OVERWRITE = 0x026;
        const unsigned short FEATURE_CD_RW_CAV_WRITE = 0x027;
        const unsigned short FEATURE_MRW = 0x028;
        const unsigned short FEATURE_ENHANCED_DEFECT_REPORTING = 0x029;
        const unsigned short FEATURE_DVD_PLUS_RW = 0x02A;
        const unsigned short FEATURE_DVD_PLUS_R = 0x02B;
        const unsigned short FEATURE_RIGID_RESTRICTED_OVERWRITE = 0x02C;
        const unsigned short FEATURE_CD_TRACK_AT_ONCE = 0x02D;
        const unsigned short FEATURE_CD_MASTERING = 0x02E;
        const unsigned short FEATURE_DVD_R_RW_WRITE = 0x02F;
        const unsigned short FEATURE_DDCD_READ = 0x030;
        const unsigned short FEATURE_DDCD_R_WRITE = 0x031;
        const unsigned short FEATURE_DDCD_RW_WRITE = 0x032;
        const unsigned short FEATURE_LAYER_JUMP_RECORDING = 0x033;
        const unsigned short FEATURE_CD_RW_MEDIA_WRITE_SUPPORT = 0x037;
        const unsigned short FEATURE_BD_PSEUDO_OVERWRITE = 0x038;
        const unsigned short FEATURE_DVD_PLUS_RW_DUAL_LAYER = 0x03A;            /**< since MMC5 revision 3 */
        const unsigned short FEATURE_DVD_PLUS_R_DUAL_LAYER = 0x03B;
        const unsigned short FEATURE_BD_READ = 0x040;
        const unsigned short FEATURE_BD_WRITE = 0x041;
        const unsigned short FEATURE_HD_DVD_READ = 0x050;
        const unsigned short FEATURE_HD_DVD_WRITE = 0x051;
        const unsigned short FEATURE_POWER_MANAGEMENT = 0x100;
        const unsigned short FEATURE_EMBEDDED_CHANGER = 0x102;
        const unsigned short FEATURE_CD_AUDIO_ANALOG_PLAY = 0x103;
        const unsigned short FEATURE_MICROCODE_UPGRADE = 0x104;
        const unsigned short FEATURE_TIMEOUT = 0x105;
        const unsigned short FEATURE_DVD_CSS = 0x106;
        const unsigned short FEATURE_REAL_TIME_STREAMING = 0x107;
        const unsigned short FEATURE_LOGICAL_UNIT_SERIAL_NUMBER = 0x108;
        const unsigned short FEATURE_DISC_CONTROL_BLOCKS = 0x10A;
        const unsigned short FEATURE_DVD_CPRM = 0x10B;
        const unsigned short FEATURE_FIRMWARE_DATE = 0x10C;

        enum Interface {
            SCSI,        /**< The device is accessed through the SCSI subsystem. */
            IDE,         /**< The device is accessed through the IDE (ATAPI) interface. */
            OTHER        /**< Unknown interface (this is not used as the DeviceManager does only handle SCSI and IDE anyway). */
        };

        /**
         * Specifies the device type. Device::type() returns a bitwise or
         * of device types.
         */
        enum DeviceType {
            DEVICE_CD_ROM = 0x1,          /**< Device reads CD-ROM media (every device in K3b supports this.) */
            DEVICE_CD_R = 0x2,            /**< Device writes CD-R media */
            DEVICE_CD_RW = 0x4,           /**< Device writes CD-RW media */
            DEVICE_DVD_ROM = 0x8,         /**< Device reads DVD-ROM media */
            DEVICE_DVD_RAM = 0x10,        /**< Device writes DVD-RAM media */
            DEVICE_DVD_R = 0x20,          /**< Device writes DVD-R media */
            DEVICE_DVD_RW = 0x40,         /**< Device writes DVD-RW media */
            DEVICE_DVD_R_DL = 0x80,       /**< Device writes DVD-R Dual Layer media */
            DEVICE_DVD_PLUS_R = 0x100,    /**< Device writes DVD+R media */
            DEVICE_DVD_PLUS_RW = 0x200,   /**< Device writes DVD+RW media */
            DEVICE_DVD_PLUS_R_DL = 0x400, /**< Device writes DVD+R Double Layer media */
            DEVICE_HD_DVD_ROM = 0x800,    /**< Device reads HD DVD-ROM media */
            DEVICE_HD_DVD_R = 0x1000,     /**< Device writes HD DVD-R media */
            DEVICE_HD_DVD_RAM = 0x2000,   /**< Device writes HD DVD-RAM media */
            DEVICE_BD_ROM = 0x4000,       /**< Device reads BD-ROM media */
            DEVICE_BD_R = 0x8000,         /**< Device writes BD-R media */
            DEVICE_BD_RE = 0x10000        /**< Device writes BD-RE media */
        };
        Q_DECLARE_FLAGS( DeviceTypes, DeviceType )

        /**
         * The different writing modes. Device::writingModes() returns a bitwise or of writing modes.
         */
        enum WritingMode {
            WRITINGMODE_SAO = 0x1,           /**< Device writes CD or DVD-R media in Session at once (also known as DAO) writing mode */
            WRITINGMODE_SAO_R96P = 0x2,      /**< Device writes CD media with packed R-W subchannels Session at once writing mode */
            WRITINGMODE_SAO_R96R = 0x4,      /**< Device writes CD media with raw R-W subchannels Session at once writing mode */
            WRITINGMODE_TAO = 0x8,           /**< Device writes CD media in Track at once writing mode */
            WRITINGMODE_RAW = 0x10,          /**< Device writes CD media in Raw writing mode */
            WRITINGMODE_RAW_R16 = 0x20,      /**< Device writes CD media with P/Q subchannels in Raw writing mode */
            WRITINGMODE_RAW_R96P = 0x40,     /**< Device writes CD media with packed R-W subchannels Raw writing mode */
            WRITINGMODE_RAW_R96R = 0x80,     /**< Device writes CD media with raw R-W subchannels Raw writing mode */
            WRITINGMODE_INCR_SEQ = 0x100,    /**< Device writes DVD-R(W) media in incremental sequential writing mode */
            WRITINGMODE_RES_OVWR = 0x200,    /**< Device writes DVD-RW media in restricted overwrite mode */
            WRITINGMODE_LAYER_JUMP = 0x400,  /**< Device writes DVD-R Dual layer media in Layer Jump writing mode */
            WRITINGMODE_RRM = 0x800,         /**< Device writes BD-R media in Random Recording Mode */
            WRITINGMODE_SRM = 0x1000,        /**< Device writes BD-R media in Sequential recording mode */
            WRITINGMODE_SRM_POW = 0x2000     /**< Device writes BD-R media in Pseudo overwrite Sequential recording mode */
        };
        Q_DECLARE_FLAGS( WritingModes, WritingMode )

        enum MediaState {
            STATE_UNKNOWN = 0x1,    /**< Media state is unknown (when an error occurred or the device is unable to determine the media state). */
            STATE_NO_MEDIA = 0x2,   /**< No media inserted. */
            STATE_COMPLETE = 0x4,   /**< The inserted media is complete. */
            STATE_INCOMPLETE = 0x8, /**< The inserted media is incomplete/appendable. */
            STATE_EMPTY = 0x10,     /**< The inserted media is empty. */
            STATE_ALL = STATE_COMPLETE|STATE_INCOMPLETE|STATE_EMPTY
        };
        Q_DECLARE_FLAGS( MediaStates, MediaState )

        enum BackGroundFormattingState {
            BG_FORMAT_INVALID = 0x0,
            BG_FORMAT_NONE = 0x1,
            BG_FORMAT_INCOMPLETE = 0x2,
            BG_FORMAT_IN_PROGRESS = 0x4,
            BG_FORMAT_COMPLETE = 0x8
        };
        Q_DECLARE_FLAGS( BackGroundFormattingStates, BackGroundFormattingState )

        /**
         * Defines the media types used throughout K3b.
         * For all groups of media a flag is defined like MEDIA_REWRITABLE_DVD.
         *
         * None of the flags is defines as 0 so we can actually include things like
         * MEDIA_NONE in flag combinations. This is important when specifying sets
         * of supported media or the like.
         */
        enum MediaType {
            /** Represents an unknown media type (when an error occurred) */
            MEDIA_UNKNOWN = 0x1,

            /** No medium is inserted. */
            MEDIA_NONE = 0x2,

            /** DVD-ROM media */
            MEDIA_DVD_ROM = 0x4,

            MEDIA_DVD_R = 0x8,

            MEDIA_DVD_R_SEQ = 0x10,

            /** Dual Layer DVD-R media. */
            MEDIA_DVD_R_DL = 0x20,

            MEDIA_DVD_R_DL_SEQ = 0x40,

            MEDIA_DVD_R_DL_JUMP = 0x80,

            MEDIA_DVD_RAM = 0x100,

            MEDIA_DVD_RW = 0x200,

            /** DVD-RW media formatted in Restricted Overwrite mode. */
            MEDIA_DVD_RW_OVWR = 0x400,

            /** DVD-RW media formatted in Incremental Sequential mode. */
            MEDIA_DVD_RW_SEQ = 0x800,

            MEDIA_DVD_PLUS_RW = 0x1000,

            MEDIA_DVD_PLUS_R = 0x2000,

            /** Double Layer DVD+R media. */
            MEDIA_DVD_PLUS_R_DL = 0x4000,

            /** Double Layer DVD+RW media. */
            MEDIA_DVD_PLUS_RW_DL = 0x8000,

            MEDIA_CD_ROM = 0x10000,
            MEDIA_CD_R = 0x20000,
            MEDIA_CD_RW = 0x40000,
            MEDIA_HD_DVD_ROM = 0x80000,
            MEDIA_HD_DVD_R = 0x100000,
            MEDIA_HD_DVD_RAM = 0x200000,

            /** Read-only Blu-ray Disc (BD) */
            MEDIA_BD_ROM = 0x400000,

            /** Writable Blu-ray Disc (BD-R) */
            MEDIA_BD_R = 0x800000,

            /** Writable Blu-ray Disc (BD-R) */
            MEDIA_BD_R_SRM = 0x1000000,

            /** Writable Blu-ray Disc (BD-R) */
            MEDIA_BD_R_SRM_POW = 0x2000000,

            /** Writable Blu-ray Disc (BD-R) */
            MEDIA_BD_R_RRM = 0x4000000,

            /** Rewritable Blu-ray Disc (BD-RE) */
            MEDIA_BD_RE = 0x8000000,

            /** This is a bitwise or of media types representing all writable CD media.*/
            MEDIA_WRITABLE_CD = MEDIA_CD_R |
            MEDIA_CD_RW,

            MEDIA_CD_ALL = MEDIA_WRITABLE_CD |
            MEDIA_CD_ROM,

            MEDIA_REWRITABLE_DVD_SL = MEDIA_DVD_RW |
            MEDIA_DVD_RW_OVWR |
            MEDIA_DVD_RW_SEQ |
            MEDIA_DVD_PLUS_RW,

            MEDIA_REWRITABLE_DVD_DL = MEDIA_DVD_PLUS_RW_DL,

            MEDIA_REWRITABLE_DVD = MEDIA_REWRITABLE_DVD_SL |
            MEDIA_DVD_PLUS_RW_DL,

            /** This is a bitwise or of media types representing all writable single layer DVD media. */
            MEDIA_WRITABLE_DVD_SL = MEDIA_REWRITABLE_DVD_SL |
            MEDIA_DVD_R |
            MEDIA_DVD_R_SEQ |
            MEDIA_DVD_RW |
            MEDIA_DVD_RW_OVWR |
            MEDIA_DVD_RW_SEQ |
            MEDIA_DVD_PLUS_RW |
            MEDIA_DVD_PLUS_R,

            /** This is a bitwise or of media types representing all writable double layer DVD media. */
            MEDIA_WRITABLE_DVD_DL = MEDIA_REWRITABLE_DVD_DL |
            MEDIA_DVD_R_DL |
            MEDIA_DVD_R_DL_SEQ |
            MEDIA_DVD_R_DL_JUMP |
            MEDIA_DVD_PLUS_R_DL |
            MEDIA_DVD_PLUS_RW_DL,

            /** This is a bitwise or of media types representing all writable DVD media. */
            MEDIA_WRITABLE_DVD = MEDIA_WRITABLE_DVD_SL |
            MEDIA_WRITABLE_DVD_DL,

            MEDIA_REWRITABLE_BD = MEDIA_BD_RE,

            /** This is a bitwise or of media types representing all writable BD media. */
            MEDIA_WRITABLE_BD = MEDIA_REWRITABLE_BD |
            MEDIA_BD_R |
            MEDIA_BD_R_SRM |
            MEDIA_BD_R_SRM_POW |
            MEDIA_BD_R_RRM,

            /** This is a bitwise or of media types representing all writable media. */
            MEDIA_WRITABLE = MEDIA_WRITABLE_CD |
            MEDIA_WRITABLE_DVD |
            MEDIA_WRITABLE_BD,

            MEDIA_REWRITABLE = MEDIA_CD_RW |
            MEDIA_REWRITABLE_DVD |
            MEDIA_REWRITABLE_BD,

            /** This is a bitwise or of media types representing all DVD-R/W media. */
            MEDIA_DVD_MINUS_ALL = MEDIA_DVD_R |
            MEDIA_DVD_R_SEQ |
            MEDIA_DVD_RW |
            MEDIA_DVD_RW_OVWR |
            MEDIA_DVD_RW_SEQ |
            MEDIA_DVD_R_DL |
            MEDIA_DVD_R_DL_SEQ |
            MEDIA_DVD_R_DL_JUMP,

            /** This is a bitwise or of media types representing all DVD+R/W media. */
            MEDIA_DVD_PLUS_ALL = MEDIA_DVD_PLUS_RW |
            MEDIA_DVD_PLUS_R |
            MEDIA_DVD_PLUS_R_DL |
            MEDIA_DVD_PLUS_RW_DL,

            MEDIA_DVD_ALL = MEDIA_WRITABLE_DVD |
            MEDIA_DVD_ROM,

            MEDIA_BD_ALL = MEDIA_WRITABLE_BD |
            MEDIA_BD_ROM,

            MEDIA_ALL = MEDIA_CD_ALL |
            MEDIA_DVD_ALL |
            MEDIA_BD_ALL
        };
        Q_DECLARE_FLAGS( MediaTypes, MediaType )

        enum SpeedMultiplicator {
            SPEED_FACTOR_CD = 175,
            SPEED_FACTOR_CD_MODE1 = 150,
            SPEED_FACTOR_DVD = 1385,
            SPEED_FACTOR_BD = 4496 // 4495.5
        };

        enum CopyrightProtectionSytemType {
            COPYRIGHT_PROTECTION_NONE = 0x0,
            COPYRIGHT_PROTECTION_CSS = 0x1,
            COPYRIGHT_PROTECTION_CPRM = 0x2,
            COPYRIGHT_PROTECTION_AACS_HD_DVD = 0x3,
            COPYRIGHT_PROTECTION_AACS_BD = 0x10
        };

        inline bool isDvdMedia( MediaTypes mediaType ) {
            return ( mediaType & MEDIA_DVD_ALL );
        }

        inline bool isCdMedia( MediaTypes mediaType ) {
            return ( mediaType & MEDIA_CD_ALL );
        }

        inline bool isBdMedia( MediaTypes mediaType ) {
            return ( mediaType & MEDIA_BD_ALL );
        }

        inline bool isRewritableMedia( MediaTypes mediaType ) {
            return ( mediaType & MEDIA_REWRITABLE );
        }
    }
}

Q_DECLARE_OPERATORS_FOR_FLAGS(K3b::Device::DeviceTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(K3b::Device::WritingModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(K3b::Device::MediaStates)
Q_DECLARE_OPERATORS_FOR_FLAGS(K3b::Device::BackGroundFormattingStates)
Q_DECLARE_OPERATORS_FOR_FLAGS(K3b::Device::MediaTypes)

#endif
