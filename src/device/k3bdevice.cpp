#include "k3bdevice.h"
#include "k3btrack.h"
#include "k3btoc.h"

#include <qstring.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
}


const char* K3bDevice::cdrdao_drivers[13] = { "auto", "plextor", "plextor-scan", "cdd2600", "generic-mmc", 
					      "generic-mmc-raw", "ricoh-mp6200", "sony-cdu920", 
					      "sony-cdu948", "taiyo-yuden", "teac-cdr55", "toshiba", 
					      "yamaha-cdr10x" };


K3bDevice::K3bDevice( cdrom_drive* drive )
{
  m_genericDevice = drive->cdda_device_name;
  m_ioctlDevice = drive->ioctl_device_name;
  m_cdrdaoDriver = "auto";
  m_cdTextCapable = 0;
  m_cdromStruct = 0;
  m_maxWriteSpeed = 0;
  m_maxReadSpeed = 0;
  m_burnproof = false;
  m_burner = false;

  m_bus = m_target = m_lun = -1;
}


K3bDevice::~K3bDevice()
{
}


cdrom_drive* K3bDevice::open()
{
  if( m_cdromStruct == 0 ) {
    m_cdromStruct = cdda_identify( devicename().latin1(), CDDA_MESSAGE_FORGETIT, 0 );
    if( !m_cdromStruct ) {
      qDebug( "(K3bDevice) Could not open device " + devicename() );
      return 0;
    }
    cdda_open( m_cdromStruct );
    return m_cdromStruct;
  }
  else
    return m_cdromStruct;
}


bool K3bDevice::close()
{
  if( m_cdromStruct == 0 )
    return false;
  else {
    cdda_close( m_cdromStruct );
    m_cdromStruct = 0;
    return true;
  }
}



const QString& K3bDevice::devicename() const
{
  if( !genericDevice().isEmpty() )
    return genericDevice();
  else
    return ioctlDevice();
}


const QString& K3bDevice::ioctlDevice() const
{
  return m_ioctlDevice;
}


const QString& K3bDevice::genericDevice() const
{
  return m_genericDevice;
}


QString K3bDevice::busTargetLun() const
{
  return QString("%1,%2,%3").arg(m_bus).arg(m_target).arg(m_lun);
}


int K3bDevice::cdTextCapable() const
{
  if( cdrdaoDriver() == "auto" )
    return 0;
  else
    return m_cdTextCapable;
}


void K3bDevice::setCdTextCapability( bool b )
{
  m_cdTextCapable = ( b ? 1 : 2 );
}


void K3bDevice::setMountPoint( const QString& mp )
{
  m_mountPoint = mp;
}


void K3bDevice::setBurnproof( bool b )
{
  m_burnproof = b;
}


int K3bDevice::isReady() const
{
  return 0;
}


int K3bDevice::isEmpty()
{
  // per default we can only differ between empty and complete
  cdrom_drive* drive = open();
  if( !drive )
    return -1;

  int t = drive->tracks;

  close();

  return ( t > 0 ? 2 : 0 );
}


bool K3bDevice::block( bool block ) const
{
  return false;
}


K3bToc K3bDevice::readToc()
{
  cdrom_drive* drive = open();
  if( !drive )
    return K3bToc();

  K3bToc toc;
  int discFirstSector = cdda_disc_firstsector( drive );
  toc.setFirstSector( discFirstSector );

  int tracks = cdda_tracks( drive );
  for( int i = 1; i <= tracks; i++ ) {
    
    int firstSector = cdda_track_firstsector( drive, i );
    int lastSector = cdda_track_lastsector( drive, i );
    int type = ( cdda_track_audiop( drive, i ) ? K3bTrack::AUDIO : K3bTrack::DATA );

    toc.append( new K3bTrack(firstSector, lastSector, type, QString("Track %1").arg(i)) );
  }

  close();

  return toc;
}
