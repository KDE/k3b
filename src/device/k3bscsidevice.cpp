#include "k3bscsidevice.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <kdebug.h>
#include <qfile.h>


K3bScsiDevice::K3bScsiDevice( const QString& devname )
  : K3bDevice( devname )
{
  m_burnproof     = false;
  m_maxReadSpeed  = 1;
  m_maxWriteSpeed = 1;
  m_burner        = false;
}


K3bScsiDevice::~K3bScsiDevice()
{
}





