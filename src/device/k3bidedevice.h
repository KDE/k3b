#ifndef K3B_IDE_DEVICE_H
#define K3B_IDE_DEVICE_H


#include "k3bdevice.h"

struct cdrom_drive;

class K3bIdeDevice : public K3bDevice
{
 public:
  K3bIdeDevice( cdrom_drive* );
  ~K3bIdeDevice();

  bool init();

  int interfaceType() const { return K3bDevice::IDE; }

  bool burnproof() const { return false; }
  bool writer() const { return false; }
  int maxWriteSpeed() const { return 0; }
};


#endif
