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

#include "k3bscsicommand.h"
#include "k3bdevice.h"

#include <kdebug.h>

#include <string.h>
#include <sys/ioctl.h>


QString senseKeyToString( int key )
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



K3bCdDevice::ScsiCommand::ScsiCommand( int fd )
  : m_fd(fd),
    m_device(0)
{
  clear();
}


K3bCdDevice::ScsiCommand::ScsiCommand( const K3bCdDevice::CdDevice* dev )
  : m_device(dev)
{
  clear();

  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  m_needToCloseDevice = !m_device->isOpen();

  m_fd = m_device->open();
}


K3bCdDevice::ScsiCommand::~ScsiCommand()
{
  if( m_device && m_needToCloseDevice )
    m_device->close();
}


void K3bCdDevice::ScsiCommand::clear()
{
  ::memset( &m_cmd, 0, sizeof(struct cdrom_generic_command) );
  ::memset( &m_sense, 0, sizeof(struct request_sense) );

  m_cmd.quiet = 1;
  m_cmd.sense = &m_sense;
}


unsigned char& K3bCdDevice::ScsiCommand::operator[]( size_t i )
{
  return m_cmd.cmd[i];
}


int K3bCdDevice::ScsiCommand::transport( TransportDirection dir,
					 void* data,
					 size_t len )
{
  if( m_fd == -1 ) {
    return -1;
  }

  m_cmd.buffer = (unsigned char*)data;
  m_cmd.buflen = len;
  if( dir == TR_DIR_READ )
    m_cmd.data_direction = CGC_DATA_READ;
  else if( dir == TR_DIR_WRITE )
    m_cmd.data_direction = CGC_DATA_WRITE;
  else
    m_cmd.data_direction = CGC_DATA_NONE;

  if( ::ioctl( m_fd, CDROM_SEND_PACKET, &m_cmd ) ) {
    kdDebug() << "(K3bCdDevice::ScsiCommand) failed: fd: " << m_fd << endl
	      << "                           command:    " << QString("%1 (%2)")
      .arg( MMC::commandString( m_cmd.cmd[0] ) )
      .arg( QString::number(m_cmd.cmd[0], 16) ) << endl
	      << "                           errorcode:  " << QString::number((int)m_sense.error_code, 16) << endl
	      << "                           sense key:  " << senseKeyToString(m_sense.sense_key) << endl
	      << "                           asc:        " << QString::number((int)m_sense.asc, 16) << endl
	      << "                           ascq:       " << QString::number((int)m_sense.ascq, 16) << endl;

    return( m_sense.error_code != 0 ? m_sense.error_code : -1 );
  }
  else
    return 0;
}


QString K3bCdDevice::MMC::commandString( const unsigned char& command )
{
  if( command == MMC::BLANK )
    return "BLANK";
  if( command == MMC::CLOSE_TRACK_SESSION )
    return "CLOSE TRACK/SESSION";
  if( command == MMC::ERASE )
    return "ERASE";
  if( command == MMC::FORMAT_UNIT )
    return "FORMAT UNIT";
  if( command == MMC::GET_CONFIGURATION )
    return "GET CONFIGURATION";
  if( command == MMC::GET_EVENT_STATUS_NOTIFICATION )
    return "GET EVENT STATUS NOTIFICATION";
  if( command == MMC::GET_PERFORMANCE )
    return "GET PERFORMANCE";
  if( command == MMC::INQUIRY )
    return "INQUIRY";
  if( command == MMC::LOAD_UNLOAD_MEDIUM )
    return "LOAD/UNLOAD MEDIUM";
  if( command == MMC::MECHANISM_STATUS )
    return "MECHANISM STATUS";
  if( command == MMC::MODE_SELECT )
    return "MODE SELECT";
  if( command == MMC::MODE_SENSE )
    return "MODE SENSE";
  if( command == MMC::PAUSE_RESUME )
    return "PAUSE/RESUME";
  if( command == MMC::PLAY_AUDIO_10 )
    return "PLAY AUDIO (10)";
  if( command == MMC::PLAY_AUDIO_12 )
    return "PLAY AUDIO (12)";
  if( command == MMC::PLAY_AUDIO_MSF )
    return "PLAY AUDIO (MSF)";
  if( command == MMC::PREVENT_ALLOW_MEDIUM_REMOVAL )
    return "PREVENT ALLOW MEDIUM REMOVAL";
  if( command == MMC::READ_10 )
    return "READ (10)";
  if( command == MMC::READ_12 )
    return "READ (12)";
  if( command == MMC::READ_BUFFER )
    return "READ BUFFER";
  if( command == MMC::READ_BUFFER_CAPACITY )
    return "READ BUFFER CAPACITY";
  if( command == MMC::READ_CAPACITY )
    return "READ CAPACITY";
  if( command == MMC::READ_CD )
    return "READ CD";
  if( command == MMC::READ_CD_MSF )
    return "READ CD MSF";
  if( command == MMC::READ_DISK_INFORMATION )
    return "READ DISK INFORMATION";
  if( command == MMC::READ_DVD_STRUCTURE )
    return "READ DVD STRUCTURE";
  if( command == MMC::READ_FORMAT_CAPACITIES )
    return "READ FORMAT CAPACITIES";
  if( command == MMC::READ_SUB_CHANNEL )
    return "READ SUB-CHANNEL";
  if( command == MMC::READ_TOC_PMA_ATIP )
    return "READ TOC/PMA/ATIP";
  if( command == MMC::READ_TRACK_INFORMATION )
    return "READ TRACK INFORMATION";
  if( command == MMC::REPAIR_TRACK )
    return "REPAIR TRACK";
  if( command == MMC::REPORT_KEY )
    return "REPORT KEY";
  if( command == MMC::REQUEST_SENSE )
    return "REQUEST SENSE";
  if( command == MMC::RESERVE_TRACK )
    return "RESERVE TRACK";
  if( command == MMC::SCAN )
    return "SCAN";
  if( command == MMC::SEEK_10 )
    return "SEEK (10)";
  if( command == MMC::SEND_CUE_SHEET )
    return "SEND CUE SHEET";
  if( command == MMC::SEND_DVD_STRUCTURE )
    return "SEND DVD STRUCTURE";
  if( command == MMC::SEND_KEY )
    return "SEND KEY";
  if( command == MMC::SEND_OPC_INFORMATION )
    return "SEND OPC INFORMATION";
  if( command == MMC::SET_SPEED )
    return "SET SPEED";
  if( command == MMC::SET_READ_AHEAD )
    return "SET READ AHEAD";
  if( command == MMC::SET_STREAMING )
    return "SET STREAMING";
  if( command == MMC::START_STOP_UNIT )
    return "START STOP UNIT";
  if( command == MMC::STOP_PLAY_SCAN )
    return "STOP PLAY/SCAN";
  if( command == MMC::SYNCHRONIZE_CACHE )
    return "SYNCHRONIZE CACHE";
  if( command == MMC::TEST_UNIT_READY )
    return "TEST UNIT READY";
  if( command == MMC::VERIFY_10 )
    return "VERIFY (10)";
  if( command == MMC::WRITE_10 )
    return "WRITE (10)";
  if( command == MMC::WRITE_12 )
    return "WRITE (12)";
  if( command == MMC::WRITE_AND_VERIFY_10 )
    return "WRITE AND VERIFY (10)";
  if( command == MMC::WRITE_BUFFER )
    return "WRITE BUFFER";

  return "unknown";
}
