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

#ifndef _K3B_EXCEPTIONS_H_
#define _K3B_EXCEPTIONS_H_

namespace K3bCdDevice {
  class CdDevice;
}

class K3bExceptions
{
 public:
  /**
   * Returns true if the drive's firmware produces broken
   * Audio CDs with zero length pregaps.
   *
   * It simply uses a compiled in table.
   */
  static bool brokenDaoAudio( K3bCdDevice::CdDevice* );
};

#endif
