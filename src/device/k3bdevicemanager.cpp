#include "k3bdevicemanager.h"
#include "k3bidedevice.h"
#include "k3bscsidevice.h"


#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>

#include <kprocess.h>
#include <kapp.h>
#include <kconfig.h>

#include <iostream>

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
}


const char* K3bDeviceManager::deviceNames[] =
  { "/dev/cdrom", "/dev/cdrecorder", "/dev/dvd", "/dev/sg0", "/dev/sg1", "/dev/sg2", "/dev/sg3", "/dev/sg4", "/dev/sg5",
    "/dev/sg6", "/dev/sg7", "/dev/sg8", "/dev/sg9", "/dev/sg10", "/dev/sg11", "/dev/sg12", "/dev/sg13",
    "/dev/sg14", "/dev/sg15" };




K3bDeviceManager::K3bDeviceManager( QObject * parent )
  :  QObject( parent )
{
  m_reader.setAutoDelete( true );
  m_writer.setAutoDelete( true );
}


K3bDevice* K3bDeviceManager::deviceByName( const QString& name )
{
  for( K3bDevice* _dev = m_writer.first(); _dev; _dev = m_writer.next() )
    if( _dev->devicename() == name )
      return _dev;

  for( K3bDevice * _dev = m_reader.first(); _dev; _dev = m_reader.next() )
    if( _dev->devicename() == name )
      return _dev;

  return 0;
}


K3bDeviceManager::~K3bDeviceManager()
{
}


QList<K3bDevice>& K3bDeviceManager::burningDevices()
{
  return m_writer;
}


QList<K3bDevice>& K3bDeviceManager::readingDevices()
{
  return m_reader;
}


int K3bDeviceManager::scanbus()
{
  m_foundDevices = 0;
  for( int i = 0; i < DEV_ARRAY_SIZE; i++ ) {
    if( K3bDevice *dev = scanDevice( deviceNames[i] ) ) {
      if( dev->burner() ) {
	m_writer.append( dev );
      } 
      else {
	m_reader.append( dev );
      }
      m_foundDevices++;
    }
  }

  return m_foundDevices;
}


void K3bDeviceManager::printDevices()
{
  cout << "\nReader:" << endl;
  for( K3bDevice * dev = m_reader.first(); dev != 0; dev = m_reader.next() ) {
    cout << "  " << ": " << dev->devicename() << " " << dev->vendor() << " " 
	 << dev->description() << " " << dev->version() << endl;
  }
  cout << "\nWriter:" << endl;
  for( K3bDevice * dev = m_writer.first(); dev != 0; dev = m_writer.next() ) {
    cout << "  " << ": " << dev->devicename() << " " << dev->vendor() << " " 
	 << dev->description() << " " << dev->version() << " " << dev->maxWriteSpeed() << endl;
  }
  cout << flush;
}


void K3bDeviceManager::clear()
{
    // clear current devices
  m_reader.clear();
  m_writer.clear();
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
      if( (dev = scanDevice( list[0] )) != 0 ) {
	// new device found, add to list
	if( dev->burner() )
	  m_writer.append( dev );
	else
	  m_reader.append( dev );
      }

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
      if( (dev = scanDevice( list[0] )) != 0 ) {
	// new device found, add to list
	if( dev->burner() )
	  m_writer.append( dev );
	else
	  m_reader.append( dev );
      }

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


  return true;
}


bool K3bDeviceManager::saveConfig( KConfig* c )
{
  int i = 1;
  K3bDevice* dev = m_reader.first();
  while( dev != 0 ) {
    QStringList list;
    list << dev->devicename() << QString::number(dev->maxReadSpeed());

    c->writeEntry( QString("Reader%1").arg(i), list );

    i++;
    dev = m_reader.next();
  }

  i = 1;
  dev = m_writer.first();
  while( dev != 0 ) {
    QStringList list;
    list << dev->devicename()
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


K3bDevice* K3bDeviceManager::scanDevice( const char *dev )
{
  cdrom_drive *drive = cdda_identify( dev, CDDA_MESSAGE_FORGETIT, 0 );
  if( drive == 0 ) {
    qDebug( "(K3bDeviceManager) %s could not be opened.", dev );
    return 0;
  }

  if( deviceByName( drive->cdda_device_name ) != 0 ) {
    qDebug( "(K3bDeviceManager) %s already detected as %s.", dev, drive->cdda_device_name );
    cdda_close( drive );
    return 0;
  }

  if( drive->interface == GENERIC_SCSI ) {
    K3bScsiDevice* newDevice = new K3bScsiDevice( drive );
    cdda_close( drive );
    if( newDevice->init() )
      return newDevice;
    else {
      qDebug("(K3bDeviceManager) Could not initialize device %s.", newDevice->devicename().latin1() );
      delete newDevice;
      return 0;
    }
  }
  else if( drive->interface == COOKED_IOCTL ) {
    K3bIdeDevice* newDevice = new K3bIdeDevice( drive );
    cdda_close( drive );
    if( newDevice->init() )
      return newDevice;
    else {
      qDebug("(K3bDeviceManager) Could not initialize device %s.", newDevice->devicename().latin1() );
      delete newDevice;
      return 0;
    }
  }
  else {
    qDebug( "(K3bDeviceManager) %s is not generic-scsi or cooked-ioctl.", dev );
    return 0;
  }
}


K3bDevice* K3bDeviceManager::addDevice( const QString& devicename )
{
  K3bDevice *dev = scanDevice( devicename.latin1() );

  if( dev == 0 )
    return 0;

  if( dev->burner() )
    m_writer.append( dev );
  else
    m_reader.append( dev );

  return dev;
}
