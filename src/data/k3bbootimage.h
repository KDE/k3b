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


#ifndef K3B_BOOTIMAGE_H
#define K3B_BOOTIMAGE_H

#include <qstring.h>

#include "k3bfileitem.h"

class K3bBootImage
{
 public:
  enum imageType { FLOPPY, HARDDISK };

  K3bBootImage() {
    loadSegment = loadSize = -1;
    noEmulate = noBoot = bootInfoTable = false;
    fileItem = 0;
    imageType = FLOPPY;
  }

  ~K3bBootImage() {
    delete fileItem;
  }

  bool noEmulate;
  bool noBoot;
  bool bootInfoTable;
  int loadSegment;
  int loadSize;
  int imageType;

  K3bFileItem* fileItem;
};

#endif
