#ifndef K3B_IDE_DEVICE_H
#define K3B_IDE_DEVICE_H


#include "k3bdevice.h"

struct cdrom_drive;
class QString;


class K3bIdeDevice : public K3bDevice
{
 public:
  K3bIdeDevice( cdrom_drive* );
  ~K3bIdeDevice();

  int interfaceType() const { return K3bDevice::IDE; }

  bool burnproof() const { return false; }
  bool writer() const { return false; }
  int maxWriteSpeed() const { return 0; }
  const QString& genericDevice() const;

 private:
  QString m_emptyString;  // only used for returning an empty genericDevice

  friend class K3bDeviceManager;
};


#endif
