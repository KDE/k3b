#include "k3bdevicemanager.h"
#include "k3bdevice.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>

#include <kprocess.h>
#include <kapp.h>
#include <kconfig.h>

#include <iostream>


static const char* deviceNames[] =
  { "/dev/sg0", "/dev/sg1", "/dev/sg2", "/dev/sg3", "/dev/sg4", "/dev/sg5",
    "/dev/sg6", "/dev/sg7", "/dev/sg8", "/dev/sg9", "/dev/sg10", "/dev/sg11", "/dev/sg12", "/dev/sg13",
    "/dev/sg14", "/dev/sg15" };





K3bDeviceManager::K3bDeviceManager( QObject * parent )
  :  QObject( parent ), m_reader(), m_writer()
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


int K3bDeviceManager::readConfig()
{
    KConfig* c = kapp->config();
    m_foundDevices = 0;

    if( c->hasGroup( "Devices" ) ) {
      c->setGroup( "Devices" );

      // read Readers
      QStringList list = c->readListEntry( "Reader1" );
      int devNum = 1;
      while( !list.isEmpty() ) {
	// create K3bDevice
	if( list.count() < 5 )
	  qDebug ( "(K3bDeviceManager) Corrupt entry in Kconfig file" );
	else {
	  m_reader.append( new K3bDevice( list[0], list[1],
					  list[2], false, false,
					  list[3].toInt(), list[4], 0 ) );
	  m_foundDevices++;
	}
	devNum++;
	list = c->readListEntry( QString( "Reader%1" ).arg( devNum ) );
      }

      // read Writers
      list = c->readListEntry( "Writer1" );
      devNum = 1;
      while( !list.isEmpty() ) {
	// create K3bDevice
	if( list.count() < 7 )
	  qDebug( "(K3bDeviceManager) Corrupt entry in Kconfig file" );
	else {
	  m_writer.append( new K3bDevice ( list[0], list[1],
					   list[2], true, ( list[5] == "yes" ),
					   list[3].toInt(), list[6],
					   list[4].toInt() ) );
	  m_foundDevices++;
	}
	devNum++;
	list = c->readListEntry( QString( "Writer%1" ).arg( devNum ) );
      }
    }

    return m_foundDevices;

    // TODO: check for multible devices on one bus:target:lun
}


K3bDevice* K3bDeviceManager::scanDevice( const char *dev, int showErrorMsg )
{
  K3bDevice* newDevice = new K3bDevice( dev );

  if( !newDevice->init() ) {
    delete newDevice;
    return 0;
  }
  else
    return newDevice;
}


