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


#ifndef _K3B_DEVICE_TYPES_H_
#define _K3B_DEVICE_TYPES_H_

namespace K3bDevice {
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
    SCSI,
    IDE,
    OTHER
  };

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
  // TODO: remove this and also use MediaType
  enum DeviceType {
    CDR = 1,
    CDRW = 2,
    CDROM = 4,
    DVD = 8,
    DVDRAM = 16,
    DVDR = 32,
    DVDRW = 64,
    DVDPR = 128,
    DVDPRW = 256
  };


  /**
   * The different writing modes.
   */
  enum WriteMode {
    SAO = 1,
    TAO = 2,
    RAW = 4,
    PACKET = 8,
    SAO_R96P = 16,
    SAO_R96R = 32,
    RAW_R16 = 64,
    RAW_R96P = 128,
    RAW_R96R = 256
  };


  enum MediaState { 
    STATE_UNKNOWN = 256,
    STATE_NO_MEDIA = 0,
    STATE_COMPLETE = 1, 
    STATE_INCOMPLETE = 2,
    STATE_EMPTY = 4
  };

  enum BackGroundFormattingState {
    BG_FORMAT_NONE = 0,
    BG_FORMAT_INCOMPLETE = 1,
    BG_FORMAT_IN_PROGRESS = 2,
    BG_FORMAT_COMPLETE = 3
  };

  /**
   * Defines the different media types as retured by 
   * K3bDevice::Device::mediaType()
   */
  enum MediaType {
    MEDIA_NONE = 0x0,
    MEDIA_DVD_ROM = 0x1,
    MEDIA_DVD_R = 0x2,
    MEDIA_DVD_R_SEQ = 0x4,
    MEDIA_DVD_R_DL = 0x8,
    MEDIA_DVD_R_DL_SEQ = 0x10,
    MEDIA_DVD_R_DL_JUMP = 0x20,
    MEDIA_DVD_RAM = 0x40,
    MEDIA_DVD_RW = 0x80,
    MEDIA_DVD_RW_OVWR = 0x100,
    MEDIA_DVD_RW_SEQ = 0x200,
    MEDIA_DVD_PLUS_RW = 0x400,
    MEDIA_DVD_PLUS_R = 0x800,
    MEDIA_DVD_PLUS_R_DL = 0x1000,
    MEDIA_CD_ROM = 0x2000,
    MEDIA_CD_R = 0x4000,
    MEDIA_CD_RW = 0x8000,
    MEDIA_WRITABLE_CD = MEDIA_CD_R | 
                        MEDIA_CD_RW,
    MEDIA_WRITABLE_DVD_SL = MEDIA_DVD_R | 
                            MEDIA_DVD_R_SEQ | 
                            MEDIA_DVD_RW |
                            MEDIA_DVD_RW_OVWR |
                            MEDIA_DVD_RW_SEQ |
                            MEDIA_DVD_PLUS_RW |
                            MEDIA_DVD_PLUS_R,
    MEDIA_WRITABLE_DVD_DL = MEDIA_DVD_R_DL |
                            MEDIA_DVD_R_DL_SEQ |
                            MEDIA_DVD_R_DL_JUMP |
                            MEDIA_DVD_PLUS_R_DL,
    MEDIA_WRITABLE_DVD = MEDIA_WRITABLE_DVD_SL |
                         MEDIA_WRITABLE_DVD_DL,
    MEDIA_UNKNOWN = 0x1000000
  };

  inline bool isDvdMedia( int mediaType ) {
    return ( mediaType == MEDIA_DVD_ROM || 
	     mediaType == MEDIA_DVD_R || 
	     mediaType == MEDIA_DVD_R_SEQ || 
	     mediaType == MEDIA_DVD_RW || 
	     mediaType == MEDIA_DVD_RW_OVWR || 
	     mediaType == MEDIA_DVD_RW_SEQ || 
	     mediaType == MEDIA_DVD_PLUS_RW || 
	     mediaType == MEDIA_DVD_PLUS_R ||
	     mediaType == MEDIA_DVD_PLUS_R_DL );
  }

  inline bool isRewritableMedia( int mediaType ) {
    return ( mediaType == MEDIA_DVD_RW ||
	     mediaType == MEDIA_DVD_RW_OVWR || 
	     mediaType == MEDIA_DVD_RW_SEQ || 
	     mediaType == MEDIA_DVD_PLUS_RW || 
	     mediaType == MEDIA_CD_RW );
  }
}

#endif
