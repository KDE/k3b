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

#ifndef _K3B_LIBDVDCSS_H_
#define _K3B_LIBDVDCSS_H_

namespace K3bDevice {
  class Device;
}


/**
 * Wrapper class for libdvdcss. dynamically openes the library if it
 * is available on the system.
 */
class K3bLibDvdCss
{
 public:
  ~K3bLibDvdCss();

  /**
   * Try to open a Video DVD and authenticate it.
   * @return true if the Video DVD could be authenticated succesfully, false otherwise.
   */
  bool open( K3bDevice::Device* dev );
  void close();

  /**
   * returns 0 if the libdvdcss could not
   * be found on the system.
   * Otherwise you have to take care of
   * deleting.
   */
  static K3bLibDvdCss* create();

 private:
  class Private;
  Private* d;

  K3bLibDvdCss();

  static void* s_libDvdCss;
  static int s_counter;
};

#endif
