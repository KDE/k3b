#ifndef K3B_IDE_DEVICE_H
#define K3B_IDE_DEVICE_H


#include "k3bdevice.h"


class QString;


class K3bIdeDevice : public K3bDevice
{
 public:
  K3bIdeDevice( const QString& );
  ~K3bIdeDevice();

  int interfaceType() const { return K3bDevice::IDE; }

#ifdef SUPPORT_IDE
  QString busTargetLun() const;
#else
  bool burnproof() const { return false; }
  bool writer() const { return false; }
  int maxWriteSpeed() const { return 0; }
#endif

 protected:
  bool furtherInit();

 private:
  friend class K3bDeviceManager;
};


#endif
