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

#ifndef _K3B_SCSI_COMMAND_H_
#define _K3B_SCSI_COMMAND_H_

#include <qglobal.h>
#include <qstring.h>

#include <sys/types.h>
#ifdef Q_OS_LINUX
/*
**  Linux specific part.
*/
#  undef __STRICT_ANSI__
#  include <linux/cdrom.h>
#  define __STRICT_ANSI__
#endif

#ifdef Q_OS_FREEBSD
/*
** FreeBSD specific part.
*/
#  include <sys/types.h>
#  include <stdio.h>
#  include <camlib.h>
#  undef INQUIRY
#  undef READ_10
#  undef READ_12
#  undef READ_BUFFER
#  undef READ_CAPACITY
#  undef REQUEST_SENSE
#  undef START_STOP_UNIT
#  undef SYNCHRONIZE_CACHE
#  undef TEST_UNIT_READY
#  undef WRITE_10
#  undef WRITE_12
#  undef WRITE_BUFFER
#endif




namespace K3bCdDevice
{
  class CdDevice;

  namespace MMC {
    const unsigned char BLANK = 0xA1;
    const unsigned char CLOSE_TRACK_SESSION = 0x5B;
    const unsigned char ERASE = 0x2C;
    const unsigned char FORMAT_UNIT = 0x04;
    const unsigned char GET_CONFIGURATION = 0x46;
    const unsigned char GET_EVENT_STATUS_NOTIFICATION = 0x4A;
    const unsigned char GET_PERFORMANCE = 0xAC;
    const unsigned char INQUIRY = 0x12;
    const unsigned char LOAD_UNLOAD_MEDIUM = 0xA6;
    const unsigned char MECHANISM_STATUS = 0xBD;
    const unsigned char MODE_SELECT = 0x55;
    const unsigned char MODE_SENSE = 0x5A;
    const unsigned char PAUSE_RESUME = 0x4B;
    const unsigned char PLAY_AUDIO_10 = 0x45;
    const unsigned char PLAY_AUDIO_12 = 0xA5;
    const unsigned char PLAY_AUDIO_MSF = 0x47;
    const unsigned char PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E;
    const unsigned char READ_10 = 0x28;
    const unsigned char READ_12 = 0xA8;
    const unsigned char READ_BUFFER = 0x3C;
    const unsigned char READ_BUFFER_CAPACITY = 0x5C;
    const unsigned char READ_CAPACITY = 0x25;
    const unsigned char READ_CD = 0xBE;
    const unsigned char READ_CD_MSF = 0xB9;
    const unsigned char READ_DISK_INFORMATION = 0x51;
    const unsigned char READ_DVD_STRUCTURE = 0xAD;
    const unsigned char READ_FORMAT_CAPACITIES = 0x23;
    const unsigned char READ_SUB_CHANNEL = 0x42;
    const unsigned char READ_TOC_PMA_ATIP = 0x43;
    const unsigned char READ_TRACK_INFORMATION = 0x52;
    const unsigned char REPAIR_TRACK = 0x58;
    const unsigned char REPORT_KEY = 0xA4;
    const unsigned char REQUEST_SENSE = 0x03;
    const unsigned char RESERVE_TRACK = 0x53;
    const unsigned char SCAN = 0xBA;
    const unsigned char SEEK_10 = 0x2B;
    const unsigned char SEND_CUE_SHEET = 0x5D;
    const unsigned char SEND_DVD_STRUCTURE = 0xBF;
    const unsigned char SEND_KEY = 0xA3;
    const unsigned char SEND_OPC_INFORMATION = 0x54;
    const unsigned char SET_SPEED = 0xBB;
    const unsigned char SET_READ_AHEAD = 0xA7;
    const unsigned char SET_STREAMING = 0xB6;
    const unsigned char START_STOP_UNIT = 0x1B;
    const unsigned char STOP_PLAY_SCAN = 0x4E;
    const unsigned char SYNCHRONIZE_CACHE = 0x35;
    const unsigned char TEST_UNIT_READY = 0x00;
    const unsigned char VERIFY_10 = 0x2F;
    const unsigned char WRITE_10 = 0x2A;
    const unsigned char WRITE_12 = 0xAA;
    const unsigned char WRITE_AND_VERIFY_10 = 0x2E;
    const unsigned char WRITE_BUFFER = 0x3B;

    QString commandString( const unsigned char& command );
  }


  enum TransportDirection {
    TR_DIR_NONE,
    TR_DIR_READ,
    TR_DIR_WRITE
  };

  class ScsiCommand
    {
    public:
      ScsiCommand( int fd );
      ScsiCommand( const CdDevice* );
#ifdef Q_OS_FREEBSD
      ScsiCommand( const CdDevice* , struct cam_device *); // FBSD
#endif
      ~ScsiCommand();

      void clear();

      unsigned char& operator[]( size_t );

      int transport( TransportDirection dir = TR_DIR_NONE,
		     void* = 0,
		     size_t len = 0 );

    private:
      // The private class holds OS-specific things that would
      // otherwise be member variables. There are parts of ScsiCommand
      // that refer directly to Private's members, so those are also
      // OS-specific.
      class Private;
      Private *d;
      int m_fd;
      const CdDevice* m_device;
      bool m_needToCloseDevice;
    };
}

#endif
