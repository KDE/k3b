#include "k3bdevicemanager.h"
#include "k3bidedevice.h"
#include "k3bscsidevice.h"

#include "../tools/k3bexternalbinmanager.h"
#include "../tools/k3bglobals.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>

#include <kprocess.h>
#include <kapp.h>
#include <kconfig.h>

#include <iostream>
#include <fstab.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/../scsi/scsi.h> /* cope with silly includes */


typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
}


const char* K3bDeviceManager::deviceNames[] =
  { "/dev/cdrom", "/dev/cdrecorder", "/dev/dvd", "/dev/sg0", "/dev/sg1", "/dev/sg2", "/dev/sg3", "/dev/sg4", "/dev/sg5",
    "/dev/sg6", "/dev/sg7", "/dev/sg8", "/dev/sg9", "/dev/sg10", "/dev/sg11", "/dev/sg12", "/dev/sg13",
    "/dev/sg14", "/dev/sg15" };




K3bDeviceManager::K3bDeviceManager( K3bExternalBinManager* exM, QObject * parent )
  : QObject( parent ), m_externalBinManager( exM )
{
  m_reader.setAutoDelete( true );
  m_writer.setAutoDelete( true );
  m_allDevices.setAutoDelete( false );
}


K3bDevice* K3bDeviceManager::deviceByName( const QString& name )
{
  for( K3bDevice* _dev = m_allDevices.first(); _dev; _dev = m_allDevices.next() )
    if( _dev->genericDevice() == name || _dev->ioctlDevice() == name )
      return _dev;

  return 0;
}


K3bDeviceManager::~K3bDeviceManager()
{
}


QPtrList<K3bDevice>& K3bDeviceManager::burningDevices()
{
  return m_writer;
}


QPtrList<K3bDevice>& K3bDeviceManager::readingDevices()
{
  return m_reader;
}


QPtrList<K3bDevice>& K3bDeviceManager::allDevices()
{
  return m_allDevices;
}


int K3bDeviceManager::scanbus()
{
  m_foundDevices = 0;
  for( int i = 0; i < DEV_ARRAY_SIZE; i++ ) {
    if( addDevice( deviceNames[i] ) )
      m_foundDevices++;
  }

  scanFstab();

  return m_foundDevices;
}


void K3bDeviceManager::printDevices()
{
  cout << "\nReader:" << endl;
  for( K3bDevice * dev = m_reader.first(); dev != 0; dev = m_reader.next() ) {
    cout << "  " << ": " << dev->ioctlDevice() << " " << dev->genericDevice() << " " << dev->vendor() << " " 
	 << dev->description() << " " << dev->version() << endl << "    " << dev->mountPoint() << endl;
  }
  cout << "\nWriter:" << endl;
  for( K3bDevice * dev = m_writer.first(); dev != 0; dev = m_writer.next() ) {
    cout << "  " << ": " << dev->ioctlDevice() << " " << dev->genericDevice() << " " << dev->vendor() << " " 
	 << dev->description() << " " << dev->version() << " " << dev->maxWriteSpeed() << endl
	 << "    " << dev->mountPoint() << endl;
  }
  cout << flush;
}


void K3bDeviceManager::clear()
{
    // clear current devices
  m_reader.clear();
  m_writer.clear();
  m_allDevices.clear();
}


bool K3bDeviceManager::readConfig( KConfig* c )
{
  m_foundDevices = 0;

  if( !c->hasGroup( "Devices" ) ) {
    return false;
  }

  c->setGroup( "Devices" );
    
  // read Readers
  QStringList list = c->readListEntry( "Reader1" );
  int devNum = 1;
  while( !list.isEmpty() ) {

    K3bDevice *dev;
    dev = deviceByName( list[0] );

    if( dev == 0 )
      dev = addDevice( list[0] );

    if( dev != 0 ) {
      // device found, apply changes
      if( list.count() > 1 )
	dev->setMaxReadSpeed( list[1].toInt() );
      if( list.count() > 2 )
	dev->setCdrdaoDriver( list[2] );
    }

    if( dev == 0 )
      qDebug( "(K3bDeviceManager) Could not detect saved device %s.", list[0].latin1() );

    devNum++;
    list = c->readListEntry( QString( "Reader%1" ).arg( devNum ) );
  }

  // read Writers
  list = c->readListEntry( "Writer1" );
  devNum = 1;
  while( !list.isEmpty() ) {

    K3bDevice *dev;
    dev = deviceByName( list[0] );

    if( dev == 0 )
      dev = addDevice( list[0] );

    if( dev != 0 ) {
      // device found, apply changes
      if( list.count() > 1 )
	dev->setMaxReadSpeed( list[1].toInt() );
      if( list.count() > 2 )
	dev->setMaxWriteSpeed( list[2].toInt() );
      if( list.count() > 3 )
	dev->setCdrdaoDriver( list[3] );
      if( list.count() > 4 )
	dev->setCdTextCapability( list[4] == "yes" );
    }

    if( dev == 0 )
      qDebug( "(K3bDeviceManager) Could not detect saved device %s.", list[0].latin1() );

    devNum++;
    list = c->readListEntry( QString( "Writer%1" ).arg( devNum ) );
  }

  scanFstab();

  return true;
}


bool K3bDeviceManager::saveConfig( KConfig* c )
{
  int i = 1;
  K3bDevice* dev = m_reader.first();
  while( dev != 0 ) {
    QStringList list;
    list << ( !dev->genericDevice().isEmpty() ? dev->genericDevice() : dev->ioctlDevice() )
	 << QString::number(dev->maxReadSpeed())
       	 << dev->cdrdaoDriver();

    c->writeEntry( QString("Reader%1").arg(i), list );

    i++;
    dev = m_reader.next();
  }

  i = 1;
  dev = m_writer.first();
  while( dev != 0 ) {
    QStringList list;
    list << dev->genericDevice()
	 << QString::number(dev->maxReadSpeed())
	 << QString::number(dev->maxWriteSpeed()) 
	 << dev->cdrdaoDriver();
    if( dev->cdrdaoDriver() != "auto" )
      list << ( dev->cdTextCapable() ? "yes" : "no" );

    c->writeEntry( QString("Writer%1").arg(i), list );

    i++;
    dev = m_writer.next();
  }

  return true;
}


K3bDevice* K3bDeviceManager::initializeScsiDevice( cdrom_drive* drive )
{
  K3bScsiDevice* dev = 0;

  // determine bus, target, lun
  int devFile = ::open( drive->cdda_device_name, O_RDONLY | O_NONBLOCK);
  if( devFile ) {
    struct ScsiIdLun {
      int id;
      int lun;
    };
    ScsiIdLun idLun;

    if ( ioctl( devFile, SCSI_IOCTL_GET_IDLUN, &idLun ) < 0) {
      qDebug( "(K3bDeviceManager) %s: Need a filename that resolves to a SCSI device (2).", drive->cdda_device_name );
      ::close( devFile );
      return 0;
    }
    else {
      dev = new K3bScsiDevice( drive );
      dev->m_target = idLun.id & 0xff;
      dev->m_lun    = (idLun.id >> 8) & 0xff;
      dev->m_bus    = (idLun.id >> 16) & 0xff;
      qDebug( "(K3bDeviceManager) bus: %i, id: %i, lun: %i", dev->m_bus, dev->m_target, dev->m_lun );
    }
    ::close( devFile );
  }
  else {
    qDebug("(K3bDeviceManager) ERROR: Could not open device " + dev->genericDevice() );
    return 0;
  }

  // now scan with cdrecord for a driver
  if( m_externalBinManager->foundBin( "cdrecord" ) ) {
    qDebug("(K3bDeviceManager) probing capabilities for device " + dev->genericDevice() );

    KProcess driverProc;
    
      // check drive capabilities
    driverProc << m_externalBinManager->binPath( "cdrecord" );
    driverProc << QString("dev=%1").arg(dev->busTargetLun());
    driverProc << "-prcap";
    
    connect( &driverProc, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotCollectStdout(KProcess*, char*, int)) );
    
    m_processOutput = "";
    
    driverProc.start( KProcess::Block, KProcess::Stdout );
    
    QStringList lines = QStringList::split( "\n", m_processOutput );

    // parse output
    for( QStringList::const_iterator it = lines.begin(); it != lines.end(); ++it ) {
      const QString& line = *it;

      if( line.startsWith("  ") ) {
	if( line.contains("write CD-R media") )
	  dev->m_burner = !line.contains( "not" );
	
	else if( line.contains("write CD-RW media") )
	  dev->m_bWritesCdrw = !line.contains( "not" );
	
	else if( line.contains("Buffer-Underrun-Free recording") )
	  dev->m_burnproof = !line.contains( "not" );
	
	else if( line.contains( "Maximum read  speed" ) )
	  dev->m_maxReadSpeed = K3b::round( line.mid( line.find(":")+1 ).toDouble() * 1000.0 / ( 2352.0 * 75.0 ) );
	
	else if( line.contains( "Maximum write speed" ) )
	  dev->m_maxWriteSpeed = K3b::round( line.mid( line.find(":")+1 ).toDouble() * 1000.0 / ( 2352.0 * 75.0 ) );
	
	else if( line.contains( "Buffer size" ) )
	  dev->m_bufferSize = K3b::round( line.mid( line.find(":")+1 ).toDouble() * 1024.0 / ( 2352.0 * 75.0 ) );
	else
	  qDebug("(K3bDeviceManager) unusable cdrecord output: " + line );
	
      }
      else if( line.startsWith("Vendor_info") )
	dev->m_vendor = line.mid( line.find(":")+3, 8 ).stripWhiteSpace();
      else if( line.startsWith("Identifikation") )
	dev->m_description = line.mid( line.find(":")+3, 16 ).stripWhiteSpace();
      else if( line.startsWith("Revision") )
	dev->m_version = line.mid( line.find(":")+3, 4 ).stripWhiteSpace();
      else
	qDebug("(K3bDeviceManager) unusable cdrecord output: " + line );
      
    }
    
  }


  return dev;
}


K3bDevice* K3bDeviceManager::initializeIdeDevice( cdrom_drive* drive )
{
  K3bIdeDevice* newDevice = new K3bIdeDevice( drive );
  return newDevice;
}


K3bDevice* K3bDeviceManager::addDevice( const QString& devicename )
{
  cdrom_drive *drive = cdda_identify( devicename, CDDA_MESSAGE_FORGETIT, 0 );
  if( drive == 0 ) {
    qDebug( "(K3bDeviceManager) %s could not be opened.", devicename.latin1() );
    return 0;
  }

  K3bDevice* dev = 0;
  if( deviceByName( drive->cdda_device_name ) != 0 ) {
    qDebug( "(K3bDeviceManager) %s already detected as %s.", devicename.latin1(), drive->cdda_device_name );
  }
  else {
    if( drive->interface == GENERIC_SCSI ) {
      dev = initializeScsiDevice( drive );
    }
    else if( drive->interface == COOKED_IOCTL ) {
      dev = initializeIdeDevice( drive );
    }
    else {
      qDebug( "(K3bDeviceManager) %s is not generic-scsi or cooked-ioctl.", devicename.latin1() );
    }
  }
    
  cdda_close( drive );


  if( dev == 0 )
    return 0;

  if( dev->burner() )
    m_writer.append( dev );
  else
    m_reader.append( dev );

  m_allDevices.append( dev );

  return dev;
}


void K3bDeviceManager::scanFstab()
{

  // for the mountPoints we need to use the ioctl-device name
  // since sg is no block device and so cannot be mounted

  K3bDevice* dev = m_allDevices.first();
  while( dev != 0 ) {
    // mounting only makes sense with a working ioctlDevice
    // if we do not have permission to read the device ioctlDevice is empty
    struct fstab* fs = 0;
    if( !dev->ioctlDevice().isEmpty() ) 
      fs = getfsspec( dev->ioctlDevice().latin1() );
    if( fs != 0 )
      dev->setMountPoint( fs->fs_file );

    dev = m_allDevices.next();
  }
}


void K3bDeviceManager::slotCollectStdout( KProcess*, char* data, int len )
{
  m_processOutput += QString::fromLocal8Bit( data, len );
}


#include "k3bdevicemanager.moc"
