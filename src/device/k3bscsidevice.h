
#ifndef K3B_SCSI_DEVICE_H
#define K3B_SCSI_DEVICE_H

#include "k3bdevice.h"


class K3bScsiDevice : public K3bDevice
{
 public:
  K3bScsiDevice( const QString& );
  ~K3bScsiDevice();

//  int isReady() const;
//  int isEmpty();
//  bool rewritable();

//  bool block(bool) const;

  int interfaceType() const { return K3bDevice::SCSI; }
  friend class K3bDeviceManager;
};

#endif
