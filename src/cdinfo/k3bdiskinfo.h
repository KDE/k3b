
#ifndef K3BDISKINFO_H
#define K3BDISKINFO_H


#include "../device/k3btoc.h"

#include <qstring.h>

class K3bDevice;

class K3bDiskInfo
{
 public:
  K3bDiskInfo()
    : empty(false), 
    cdrw(false), 
    appendable(false), 
    noDisk(false), 
    size(0), 
    remaining(0),
    speed(0), 
    sessions(0),
    tocType(UNKNOWN),
    valid(true),
    device(0)
    { }

  enum type { UNKNOWN, AUDIO, DATA, MIXED, DVD };

  K3bToc toc;
  QString mediumManufactor;
  QString mediumType;
  QString sizeString;
  QString remainingString;

  bool empty;
  bool cdrw;
  bool appendable;
  bool noDisk;

  unsigned long size;
  unsigned long remaining;

  int speed;
  int sessions;
  int tocType;

  // iso stuff
  QString isoId;
  QString isoSystemId;
  QString isoVolumeId;
  QString isoVolumeSetId;
  QString isoPublisherId;
  QString isoPreparerId;
  QString isoApplicationId;

  bool valid;

  K3bDevice* device;
};

#endif
