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

#ifndef _K3B_SCSI_COMMAND_H_
#define _K3B_SCSI_COMMAND_H_

#include <qglobal.h>
#include <qstring.h>

#include "k3bdevice.h"


namespace K3b {
    namespace Device
    {
        const unsigned char MMC_BLANK = 0xA1;
        const unsigned char MMC_CLOSE_TRACK_SESSION = 0x5B;
        const unsigned char MMC_ERASE = 0x2C;
        const unsigned char MMC_FORMAT_UNIT = 0x04;
        const unsigned char MMC_GET_CONFIGURATION = 0x46;
        const unsigned char MMC_GET_EVENT_STATUS_NOTIFICATION = 0x4A;
        const unsigned char MMC_GET_PERFORMANCE = 0xAC;
        const unsigned char MMC_INQUIRY = 0x12;
        const unsigned char MMC_LOAD_UNLOAD_MEDIUM = 0xA6;
        const unsigned char MMC_MECHANISM_STATUS = 0xBD;
        const unsigned char MMC_MODE_SELECT = 0x55;
        const unsigned char MMC_MODE_SENSE = 0x5A;
        const unsigned char MMC_PAUSE_RESUME = 0x4B;
        const unsigned char MMC_PLAY_AUDIO_10 = 0x45;
        const unsigned char MMC_PLAY_AUDIO_12 = 0xA5;
        const unsigned char MMC_PLAY_AUDIO_MSF = 0x47;
        const unsigned char MMC_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E;
        const unsigned char MMC_READ_10 = 0x28;
        const unsigned char MMC_READ_12 = 0xA8;
        const unsigned char MMC_READ_BUFFER = 0x3C;
        const unsigned char MMC_READ_BUFFER_CAPACITY = 0x5C;
        const unsigned char MMC_READ_CAPACITY = 0x25;
        const unsigned char MMC_READ_CD = 0xBE;
        const unsigned char MMC_READ_CD_MSF = 0xB9;
        const unsigned char MMC_READ_DISC_INFORMATION = 0x51;
        const unsigned char MMC_READ_DVD_STRUCTURE = 0xAD;
        const unsigned char MMC_READ_DISC_STRUCTURE = 0xAD; /**< READ DVD STRUCTURE has been renamed to READ DISC STRUCTURE in MMC5 */
        const unsigned char MMC_READ_FORMAT_CAPACITIES = 0x23;
        const unsigned char MMC_READ_SUB_CHANNEL = 0x42;
        const unsigned char MMC_READ_TOC_PMA_ATIP = 0x43;
        const unsigned char MMC_READ_TRACK_INFORMATION = 0x52;
        const unsigned char MMC_REPAIR_TRACK = 0x58;
        const unsigned char MMC_REPORT_KEY = 0xA4;
        const unsigned char MMC_REQUEST_SENSE = 0x03;
        const unsigned char MMC_RESERVE_TRACK = 0x53;
        const unsigned char MMC_SCAN = 0xBA;
        const unsigned char MMC_SEEK_10 = 0x2B;
        const unsigned char MMC_SEND_CUE_SHEET = 0x5D;
        const unsigned char MMC_SEND_DVD_STRUCTURE = 0xBF;
        const unsigned char MMC_SEND_KEY = 0xA3;
        const unsigned char MMC_SEND_OPC_INFORMATION = 0x54;
        const unsigned char MMC_SET_SPEED = 0xBB;
        const unsigned char MMC_SET_READ_AHEAD = 0xA7;
        const unsigned char MMC_SET_STREAMING = 0xB6;
        const unsigned char MMC_START_STOP_UNIT = 0x1B;
        const unsigned char MMC_STOP_PLAY_SCAN = 0x4E;
        const unsigned char MMC_SYNCHRONIZE_CACHE = 0x35;
        const unsigned char MMC_TEST_UNIT_READY = 0x00;
        const unsigned char MMC_VERIFY_10 = 0x2F;
        const unsigned char MMC_WRITE_10 = 0x2A;
        const unsigned char MMC_WRITE_12 = 0xAA;
        const unsigned char MMC_WRITE_AND_VERIFY_10 = 0x2E;
        const unsigned char MMC_WRITE_BUFFER = 0x3B;

        QString commandString( const unsigned char& command );

        enum TransportDirection {
            TR_DIR_NONE,
            TR_DIR_READ,
            TR_DIR_WRITE
        };

        class ScsiCommand
        {
        public:
#ifndef Q_OS_WIN32        
            ScsiCommand( Device::Handle handle );
#endif            
            ScsiCommand( const Device* );
            ~ScsiCommand();

            /**
             * Enales or disables printing of debugging messages for failed
             * commands.
             *
             * Default is enabled.
             */
            void enableErrorMessages( bool b ) { m_printErrors = b; }

            void clear();

            unsigned char& operator[]( size_t );

            // TODO: use this
/*       union ErrorCode { */
/* 	K3b::Device::quint32 code; */
/* 	struct { */
/* 	  K3b::Device::quint8 errorCode; */
/* 	  K3b::Device::quint8 senseKey; */
/* 	  K3b::Device::quint8 asc; */
/* 	  K3b::Device::quint8 ascq; */
/* 	} details; */
/*       }; */

            /**
             * \return 0 on success, -1 if the device could not be opened, and
             *         an error code otherwise. The error code is constructed from
             *         the scsi error code, the sense key, asc, and ascq. These four values are 
             *         combined into the lower 32 bit of an integer in the order used above.
             */
            int transport( TransportDirection dir = TR_DIR_NONE,
                           void* = 0,
                           size_t len = 0 );

        private:
            static QString senseKeyToString( int key );
            void debugError( int command, int errorCode, int senseKey, int asc, int ascq );

            class Private;
            Private *d;
            Device::Handle m_deviceHandle;
            const Device* m_device;

            bool m_printErrors;
        };
    }
}

#endif
