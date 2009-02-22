/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bexceptions.h"
#include <k3bdevice.h>

bool K3b::Exceptions::brokenDaoAudio( K3b::Device::Device* dev )
{
    if( dev->vendor().toUpper().startsWith("PIONEER") )
        if( dev->description().toUpper().startsWith("DVR-106D") ||
            dev->description().toUpper().startsWith("DVD-RW  DVR-K12D") )
            return true;

    if( dev->vendor().toUpper().startsWith("HL-DT-ST") )
        if( dev->description().toUpper().startsWith("RW/DVD GCC-4320B") ||
            dev->description().toUpper().contains("GCE-8520B") )
            return true;

    if( dev->vendor().toUpper().startsWith("PHILIPS") &&
        dev->description().toUpper().startsWith("CDRWDVD3210") )
        return true;

    if( dev->vendor().toUpper().startsWith("LITE-ON") )
        if( dev->description().toUpper().startsWith("LTR-32123S") ||
            dev->description().toUpper().startsWith("LTR-40125S") ||
            dev->description().toUpper().contains("LTC-48161H") ||
            dev->description().toUpper().startsWith("DVDRW LDW-811S") )
            return true;

    return false;
}
