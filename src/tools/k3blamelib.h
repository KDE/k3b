/* 
 *
 * $Id: $
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


#ifndef K3B_LAME_LIB_H
#define K3B_LAME_LIB_H


class QLibrary;

class K3bLameLib
{
 public:
  ~K3bLameLib();

  bool load();

  bool init();
  int getVersion();

  /**
   * returns 0 if the lamelib could not
   * be found on the system.
   * Otherwise you have to take care of
   * deleting.
   */
  static K3bLameLib* create();

 private:
  K3bLameLib();

  static QLibrary* s_lib;
  static int s_counter;

  class Private;
  Private* d;
};


#endif
