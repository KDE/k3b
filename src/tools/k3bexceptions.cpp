/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bexceptions.h"
#include <k3bdevice.h>

bool K3bExceptions::brokenDaoAudio( K3bCdDevice::CdDevice* dev )
{
  if( dev->vendor().upper().startsWith("PIONEER") &&
      dev->description().upper().startsWith("DVR-106D") )
    return true;

  if( dev->vendor().upper().startsWith("HL-DT-ST") &&
      dev->description().upper().startsWith("RW/DVD GCC-4320B") )
    return true;

  if( dev->vendor().upper().startsWith("PHILIPS") &&
      dev->description().upper().startsWith("CDRWDVD3210") )
    return true;

  if( dev->vendor().upper().startsWith("LITE-ON") )
    if( dev->description().upper().startsWith("LTR-32123S") ||
	dev->description().upper().startsWith("LTR-40125S") ||
	dev->description().upper().contains("LTC-48161H") )
    return true;

  return false;
}
