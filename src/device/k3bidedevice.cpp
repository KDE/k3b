#include "k3bidedevice.h"

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
}



K3bIdeDevice::K3bIdeDevice( cdrom_drive* drive )
  : K3bDevice( drive )
{
  QString model( drive->drive_model );

  // the cd_paranoia-lib puts vendor, model, and version in one string
  // we need to split it in the future

 m_description = model;

 m_burner = false;
 m_burnproof = false;
 m_maxWriteSpeed = -1;

 // we could use cdda_speed_set to test the reading speed
 // for example from 100 down to 1 until it returns TR_OK
}

K3bIdeDevice::~K3bIdeDevice()
{
}


bool K3bIdeDevice::init()
{
  // nothing to do so far
  return true;
}
