#include "k3bidedevice.h"

#include <stdlib.h>
#include <fcntl.h>		// O_RDONLY etc.
#include <linux/hdreg.h>
#include <sys/ioctl.h>		// ioctls

#include <kdebug.h>


K3bIdeDevice::K3bIdeDevice( const QString& drive )
  : K3bDevice( drive )
{
  m_burner = false;
  m_burnproof = false;
  m_maxWriteSpeed = -1;

  // we could use cdda_speed_set to test the reading speed
  // for example from 100 down to 1 until it returns TR_OK
}

K3bIdeDevice::~K3bIdeDevice()
{
}

#ifdef SUPPORT_IDE
QString K3bIdeDevice::busTargetLun() const
{
  return QString("ATAPI:%1,%2,%3").arg(m_bus).arg(m_target).arg(m_lun);
}
#endif

bool K3bIdeDevice::furtherInit()
{
 int cdromfd = ::open( devicename().latin1(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bIdeDevice) Error: could not open device." << endl;
    return false;
  }
  else {
    struct hd_driveid hdId;
    ::ioctl( cdromfd, HDIO_GET_IDENTITY, &hdId );

    m_description = QString::fromLatin1((const char*)hdId.model, 40).stripWhiteSpace();
    m_vendor = m_description.left( m_description.find( " " ) );
    m_description = m_description.mid( m_description.find(" ")+1 );
    m_version = QString::fromLatin1((const char*)hdId.fw_rev, 8).stripWhiteSpace();

    return true;
  }
 
}
