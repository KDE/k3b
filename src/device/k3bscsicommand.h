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

#ifndef _K3B_SCSI_COMMAND_H_
#define _K3B_SCSI_COMMAND_H_

#include <sys/types.h>
#undef __STRICT_ANSI__
#include <linux/cdrom.h>
#define __STRICT_ANSI__


namespace K3bCdDevice
{
  class CdDevice;


  // inspired by dvd+rw-tools
  // see transport.hxx for a BSD version

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
      ~ScsiCommand();

      void clear();

      unsigned char& operator[]( size_t );

      int transport( TransportDirection dir = TR_DIR_NONE,
		     void* = 0,
		     size_t len = 0 );

    private:
      struct cdrom_generic_command m_cmd;
      struct request_sense m_sense;

      int m_fd;
      const CdDevice* m_device;
      bool m_needToCloseDevice;
    };
}

#endif
