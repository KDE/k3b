
#ifndef K3BDISKINFO_H
#define K3BDISKINFO_H


#include "../device/k3btoc.h"

#include <qstring.h>

class K3bDevice;

class K3bDiskInfo
{
 public:
  K3bDiskInfo()
    : empty(true), 
    cdrw(false), 
    appendable(false), 
    noDisk(false), 
    size(0), 
    remaining(0),
    speed(0), 
    sessions(0),
    valid(true),
    device(0)
    { }

  enum type { AUDIO, DATA, MIXED, DVD };

  K3bToc toc;
  QString mediumManufactor;
  QString mediumType;
  QString sizeString;
  QString remainingString;
  unsigned long size;
  unsigned long remaining;
  bool cdrw;
  int speed;
  bool appendable;
  int sessions;
  bool noDisk;
  bool empty;
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
