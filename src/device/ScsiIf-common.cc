/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1999  Andreas Mueller <mueller@daneb.ping.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * $Log$
 * Revision 1.1.1.1  2000/02/05 01:36:55  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 */
#include "ScsiIf.h"
//#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>

// checks if unit is ready
// return: 0: OK, ready
//         1: not ready (busy)
//         2: not ready, no disk in drive
//         3: scsi command failed
/*int ScsiIf::testUnitReady()
{
  unsigned char cmd[6];
  const unsigned char *senseData;
  int senseLen;
  int ret = 0;

  memset(cmd, 0, 6);

  switch (sendCmd(cmd, 6, NULL, 0, NULL, 0, 0)) {
  case 1:
    ret = 3;
    break;

  case 2:
    senseData = getSense(senseLen);

    switch (senseData[2] & 0x0f) {
    case 0x02: // Not ready
      switch (senseData[12]) {
      case 0x3a: // medium not present
	ret = 2;
	break;

      default:
	ret = 1;
	break;
      }
      break;

    case 0x06: // Unit attention
      ret = 0;
      break;

    default:
      ret = 3;
      break;
    }
  }

  return ret;
}
*/