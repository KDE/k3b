
#ifndef K3B_SCSI_DEVICE_H
#define K3B_SCSI_DEVICE_H

#include "k3bdevice.h"

class ScsiIf;
struct cdrom_drive;

class K3bScsiDevice : public K3bDevice
{
 public:
  K3bScsiDevice( cdrom_drive* );
  ~K3bScsiDevice();

  int isReady() const;
  int isEmpty();
  bool rewritable();

  bool block(bool) const;

  int interfaceType() const { return K3bDevice::SCSI; }

  bool cdrecordDriver() const { return m_bCdrecordDriver; }

 private:
  int getModePage( ScsiIf *_scsiIf, int pageCode, unsigned char *buf,
		   long bufLen, unsigned char *modePageHeader,
		   unsigned char *blockDesc, int showErrorMsg );

  bool m_bCdrecordDriver;

  friend class K3bDeviceManager;
};

#endif
