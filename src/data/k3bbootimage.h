
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
