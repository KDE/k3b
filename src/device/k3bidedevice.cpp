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
  // we need to split it
  int i;
  if( (i = model.find("ATAPI")) != -1 )
    model.remove( i, 5 );
  if( (i = model.find("compatible")) != -1 )
    model.remove( i, 10 );

  model.stripWhiteSpace();

  // we assume that all letters up to the first white space 
  // belong to the vendor string and the rest is the model
  // description

  m_vendor = model.left( model.find(' ') ).stripWhiteSpace();
  m_description = model.mid( model.find(' ') ).stripWhiteSpace();
  
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
