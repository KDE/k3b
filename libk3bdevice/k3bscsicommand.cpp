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

#include "k3bscsicommand.h"
#include "k3bdevice.h"

#include <kdebug.h>


QString K3b::Device::commandString( const unsigned char& command )
{
    if( command == MMC_BLANK )
        return "BLANK";
    if( command == MMC_CLOSE_TRACK_SESSION )
        return "CLOSE TRACK/SESSION";
    if( command == MMC_ERASE )
        return "ERASE";
    if( command == MMC_FORMAT_UNIT )
        return "FORMAT UNIT";
    if( command == MMC_GET_CONFIGURATION )
        return "GET CONFIGURATION";
    if( command == MMC_GET_EVENT_STATUS_NOTIFICATION )
        return "GET EVENT STATUS NOTIFICATION";
    if( command == MMC_GET_PERFORMANCE )
        return "GET PERFORMANCE";
    if( command == MMC_INQUIRY )
        return "INQUIRY";
    if( command == MMC_LOAD_UNLOAD_MEDIUM )
        return "LOAD/UNLOAD MEDIUM";
    if( command == MMC_MECHANISM_STATUS )
        return "MECHANISM STATUS";
    if( command == MMC_MODE_SELECT )
        return "MODE SELECT";
    if( command == MMC_MODE_SENSE )
        return "MODE SENSE";
    if( command == MMC_PAUSE_RESUME )
        return "PAUSE/RESUME";
    if( command == MMC_PLAY_AUDIO_10 )
        return "PLAY AUDIO (10)";
    if( command == MMC_PLAY_AUDIO_12 )
        return "PLAY AUDIO (12)";
    if( command == MMC_PLAY_AUDIO_MSF )
        return "PLAY AUDIO (MSF)";
    if( command == MMC_PREVENT_ALLOW_MEDIUM_REMOVAL )
        return "PREVENT ALLOW MEDIUM REMOVAL";
    if( command == MMC_READ_10 )
        return "READ (10)";
    if( command == MMC_READ_12 )
        return "READ (12)";
    if( command == MMC_READ_BUFFER )
        return "READ BUFFER";
    if( command == MMC_READ_BUFFER_CAPACITY )
        return "READ BUFFER CAPACITY";
    if( command == MMC_READ_CAPACITY )
        return "READ CAPACITY";
    if( command == MMC_READ_CD )
        return "READ CD";
    if( command == MMC_READ_CD_MSF )
        return "READ CD MSF";
    if( command == MMC_READ_DISC_INFORMATION )
        return "READ DISC INFORMATION";
    if( command == MMC_READ_DVD_STRUCTURE )
        return "READ DVD STRUCTURE";
    if( command == MMC_READ_FORMAT_CAPACITIES )
        return "READ FORMAT CAPACITIES";
    if( command == MMC_READ_SUB_CHANNEL )
        return "READ SUB-CHANNEL";
    if( command == MMC_READ_TOC_PMA_ATIP )
        return "READ TOC/PMA/ATIP";
    if( command == MMC_READ_TRACK_INFORMATION )
        return "READ TRACK INFORMATION";
    if( command == MMC_REPAIR_TRACK )
        return "REPAIR TRACK";
    if( command == MMC_REPORT_KEY )
        return "REPORT KEY";
    if( command == MMC_REQUEST_SENSE )
        return "REQUEST SENSE";
    if( command == MMC_RESERVE_TRACK )
        return "RESERVE TRACK";
    if( command == MMC_SCAN )
        return "SCAN";
    if( command == MMC_SEEK_10 )
        return "SEEK (10)";
    if( command == MMC_SEND_CUE_SHEET )
        return "SEND CUE SHEET";
    if( command == MMC_SEND_DVD_STRUCTURE )
        return "SEND DVD STRUCTURE";
    if( command == MMC_SEND_KEY )
        return "SEND KEY";
    if( command == MMC_SEND_OPC_INFORMATION )
        return "SEND OPC INFORMATION";
    if( command == MMC_SET_SPEED )
        return "SET SPEED";
    if( command == MMC_SET_READ_AHEAD )
        return "SET READ AHEAD";
    if( command == MMC_SET_STREAMING )
        return "SET STREAMING";
    if( command == MMC_START_STOP_UNIT )
        return "START STOP UNIT";
    if( command == MMC_STOP_PLAY_SCAN )
        return "STOP PLAY/SCAN";
    if( command == MMC_SYNCHRONIZE_CACHE )
        return "SYNCHRONIZE CACHE";
    if( command == MMC_TEST_UNIT_READY )
        return "TEST UNIT READY";
    if( command == MMC_VERIFY_10 )
        return "VERIFY (10)";
    if( command == MMC_WRITE_10 )
        return "WRITE (10)";
    if( command == MMC_WRITE_12 )
        return "WRITE (12)";
    if( command == MMC_WRITE_AND_VERIFY_10 )
        return "WRITE AND VERIFY (10)";
    if( command == MMC_WRITE_BUFFER )
        return "WRITE BUFFER";

    return "unknown";
}


QString K3b::Device::ScsiCommand::senseKeyToString( int key )
{
    switch( key ) {
    case 0x0:
        return "NO SENSE (2)";
    case 0x1:
        return "RECOVERED ERROR (1)";
    case 0x2:
        return "NOT READY (2)";
    case 0x3:
        return "MEDIUM ERROR (3)";
    case 0x4:
        return "HARDWARE ERROR (4)";
    case 0x5:
        return "ILLEGAL REQUEST (5)";
    case 0x6:
        return "UNIT ATTENTION (6)";
    case 0x7:
        return "DATA PROTECT (7)";
    case 0x8:
        return "BLANK CHECK (8)";
    case 0x9:
        return "VENDOR SPECIFIC (9)";
    case 0xA:
        return "COPY ABORTED (A)";
    case 0xB:
        return "ABORTED COMMAND (B)";
    case 0xC:
        return "0xC is obsolete... ??";
    }

    return "unknown";
}


void K3b::Device::ScsiCommand::debugError( int command, int errorCode, int senseKey, int asc, int ascq ) {
    if( m_printErrors ) {
        kDebug() << "(K3b::Device::ScsiCommand) failed: " << endl
                 << "                           command:    " << QString("%1 (%2)")
            .arg( K3b::Device::commandString( command ) )
            .arg( QString::number(command, 16) ) << endl
                 << "                           errorcode:  " << QString::number(errorCode, 16) << endl
                 << "                           sense key:  " << senseKeyToString(senseKey) << endl
                 << "                           asc:        " << QString::number(asc, 16) << endl
                 << "                           ascq:       " << QString::number(ascq, 16) << endl;
    }
}



#ifdef Q_OS_LINUX
#include "k3bscsicommand_linux.cpp"
#endif
#ifdef Q_OS_FREEBSD
#include "k3bscsicommand_bsd.cpp"
#endif
#ifdef Q_OS_NETBSD
#include "k3bscsicommand_netbsd.cpp"
#endif
#ifdef Q_OS_WIN32
#include "k3bscsicommand_win.cpp"
#endif


#ifndef Q_OS_WIN32
K3b::Device::ScsiCommand::ScsiCommand( K3b::Device::Device::Handle handle )
    : d(new Private),
      m_device(0),
      m_printErrors(true)
{
    m_deviceHandle = handle;
    clear();
}
#endif    


K3b::Device::ScsiCommand::ScsiCommand( const K3b::Device::Device* dev )
    : d(new Private),
      m_device(dev),
      m_printErrors(true)
{
    clear();
}


K3b::Device::ScsiCommand::~ScsiCommand()
{
    delete d;
}

