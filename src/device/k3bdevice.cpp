#include "k3bdevice.h"
#include "k3btrack.h"
#include "k3btoc.h"

#include <qstring.h>
#include <qfile.h>

#include <kdebug.h>
#include <kprocess.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;


#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


const char* K3bDevice::cdrdao_drivers[] = { "auto", "plextor", "plextor-scan", "cdd2600", "generic-mmc", 
					    "generic-mmc-raw", "ricoh-mp6200", "sony-cdu920", 
					    "sony-cdu948", "taiyo-yuden", "teac-cdr55", "toshiba", 
					    "yamaha-cdr10x", 0};


class K3bDevice::Private
{
public:
  Private()
  {
  }

  QString blockDeviceName;
  QString genericDevice;
  int deviceType;
  QString mountPoint;
  QString mountDeviceName;
  QStringList allNodes;
};


K3bDevice::K3bDevice( const QString& devname )
{
  d = new Private;

  d->blockDeviceName = devname;

  d->allNodes.append(devname);

  m_cdrdaoDriver = "auto";
  m_cdTextCapable = 0;
  m_maxWriteSpeed = 0;
  m_maxReadSpeed = 0;
  m_burnproof = false;
  m_burner = false;
  m_bWritesCdrw = false;

  m_bus = m_target = m_lun = -1;


//   QString model( drive->drive_model );

//   // the cd_paranoia-lib puts vendor, model, and version in one string
//   // we need to split it
//   int i;
//   if( (i = model.find("ATAPI")) != -1 )
//     model.remove( i, 5 );
//   if( (i = model.find("compatible")) != -1 )
//     model.remove( i, 10 );

//   model = model.stripWhiteSpace();

//   // we assume that all letters up to the first white space 
//   // belong to the vendor string and the rest is the model
//   // description

//   m_vendor = model.left( model.find(' ') ).stripWhiteSpace();
//   m_description = model.mid( model.find(' ') ).stripWhiteSpace();
}


K3bDevice::~K3bDevice()
{
  delete d;
}


bool K3bDevice::init()
{
  int cdromfd = ::open( blockDeviceName().latin1(), O_RDONLY | O_NONBLOCK );
  if(cdromfd < 0) {
    kdDebug() << "could not open device " << blockDeviceName() << " (" << strerror(errno) << ")" << endl;
    return false;
  }

  int drivetype = ::ioctl(cdromfd, CDROM_GET_CAPABILITY, CDSL_CURRENT);
  if( drivetype < 0 ) {
    kdDebug() << "Error while retrieving capabilities." << endl;
    ::close( cdromfd );
    return false;
  }

  d->deviceType = CDROM;  // all drives should be able to read cdroms

  if (drivetype & CDC_CD_R) {
    d->deviceType |= CDR;
  }
  if (drivetype & CDC_CD_RW) {
    d->deviceType |= CDRW;
  }
  if (drivetype & CDC_DVD_R){
    d->deviceType |= DVDR;
  }
  if (drivetype & CDC_DVD_RAM) {
    d->deviceType |= DVDRAM;
  }
  if (drivetype & CDC_DVD) {
    d->deviceType |= DVDROM;
  }

  ::close( cdromfd );

  return furtherInit();
}


bool K3bDevice::furtherInit()
{
  return true;
}


int K3bDevice::type() const
{
  return d->deviceType;
}


const QString& K3bDevice::devicename() const
{
  return blockDeviceName();
}


const QString& K3bDevice::ioctlDevice() const
{
  return blockDeviceName();
}


const QString& K3bDevice::blockDeviceName() const
{
  return d->blockDeviceName;
}


const QString& K3bDevice::genericDevice() const
{
  return d->genericDevice;
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
  d->mountPoint = mp;
}

void K3bDevice::setMountDevice( const QString& md ) 
{ 
  d->mountDeviceName = md; 
}


const QString& K3bDevice::mountDevice() const
{ 
  return d->mountDeviceName; 
}


const QString& K3bDevice::mountPoint() const 
{
  return d->mountPoint; 
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
  int cdromfd = ::open( devicename().latin1(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return NO_INFO;
  }
  else {
    int ret = ::ioctl(cdromfd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
    int r = NO_INFO;

    if( ret == -1 ) {
      kdDebug() << "Error: " << strerror(errno) << endl;
    }
    else {
      switch(ret) {
      case CDS_DISC_OK:
	{
	  // try to determine if there are any tracks on the disk
	  struct cdrom_tochdr tocHdr;
	  if( ::ioctl(cdromfd, CDROMREADTOCHDR, &tocHdr ) ) {
	    // the disk seems to be empty
	    r = EMPTY;
	  }
	  else {
	    // there is at least a first track
	    r = COMPLETE;
	  }
    	}
	break;
      case CDS_NO_DISC:
	r = NO_DISK;
	break;
      default:
	r = NO_INFO;
      }
    }   
    
    ::close( cdromfd );
    return r;
  }
}


bool K3bDevice::block( bool ) const
{
  return false;
}


void K3bDevice::eject() const
{
  int cdromfd = ::open( devicename().latin1(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
  }
  else {
    ::ioctl( cdromfd, CDROMEJECT );
    ::close( cdromfd );
  }
}


void K3bDevice::load() const
{
  int cdromfd = ::open( devicename().latin1(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
  }
  else {
    ::ioctl( cdromfd, CDROMCLOSETRAY );
    ::close( cdromfd );
  }
}

void K3bDevice::addDeviceNode( const QString& n )
{
  if( !d->allNodes.contains( n ) )
    d->allNodes.append( n );
}


const QStringList& K3bDevice::deviceNodes() const
{
  return d->allNodes;
}


bool K3bDevice::supportsWriteMode( K3bDevice::WriteMode w )
{
  return (m_writeModes & w);
}
