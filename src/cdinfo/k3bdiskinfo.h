
#ifndef K3BDISKINFO_H
#define K3BDISKINFO_H


#include "../device/k3btoc.h"

#include <qstring.h>

class K3bDevice;

class K3bDiskInfo
{
 public:
  K3bDiskInfo()
    : empty(true), cdrw(false), appendable(false), noDisk(false), size(0), speed(0), sessions(0)
    { }

  enum type { AUDIO, DATA, MIXED, DVD };

  K3bToc toc;
  QString manufactor;
  unsigned long size;
  bool cdrw;
  int speed;
  bool appendable;
  int sessions;
  bool noDisk;
  bool empty;
  int tocType;

  K3bDevice* device;
};

#endif
