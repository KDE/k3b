#include "k3bdevice.h"
#include "../cdinfo/k3btrack.h"
#include "../cdinfo/k3btoc.h"


typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
}


const char* K3bDevice::cdrdao_drivers[13] = { "plextor", "plextor-scan", "cdd2600", "generic-mmc", 
					      "generic-mmc-raw", "ricoh-mp6200", "sony-cdu920", 
					      "sony-cdu948", "taiyo-yuden", "teac-cdr55", "toshiba", 
					      "yamaha-cdr10x", "auto" };


K3bDevice::K3bDevice( cdrom_drive* drive )
{
  m_devicename = drive->cdda_device_name;
  m_cdrdaoDriver = "auto";
  m_cdTextCapable = 0;
  m_cdromStruct = 0;
}


K3bDevice::~K3bDevice()
{
}


cdrom_drive* K3bDevice::open()
{
  if( m_cdromStruct == 0 ) {
    m_cdromStruct = cdda_identify( devicename().latin1(), CDDA_MESSAGE_FORGETIT, 0 );
    if( !m_cdromStruct ) {
      qDebug( "(K3bDevice) Could not open device " + m_devicename );
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


int K3bDevice::isReady() const
{
  return 0;
}


int K3bDevice::isEmpty() const
{
  return true;
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

    toc.append( new K3bTrack(firstSector, lastSector, type) );
  }

  close();

  return toc;
}
