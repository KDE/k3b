#include "k3bdevicemanager.h"
#include "device/ScsiIf.h"

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




K3bDevice::K3bDevice( K3bDevice* d )
{
  burner = d->burner;
  burnproof = d->burnproof;
  description = d->description;
  devicename = d->devicename;
  maxReadSpeed = d->maxReadSpeed;
  maxWriteSpeed = d->maxWriteSpeed;
  vendor = d->vendor;
  version = d->version;
}


K3bDeviceManager::K3bDeviceManager( QObject * parent )
  :  QObject( parent ), m_reader(), m_writer()
{
  m_reader.setAutoDelete( true );
  m_writer.setAutoDelete( true );

  m_scsiIf = 0;
}


K3bDevice* K3bDeviceManager::deviceByName( const QString& name )
{
  for( K3bDevice* _dev = m_writer.first(); _dev; _dev = m_writer.next() )
    if( _dev->devicename == name )
      return _dev;

  for( K3bDevice * _dev = m_reader.first(); _dev; _dev = m_reader.next() )
    if( _dev->devicename == name )
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
      if( dev->burner ) {
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
    cout << "  " << ": " << dev->devicename << " " << dev->vendor << " " 
	 << dev->description << " " << dev->version << endl;
  }
  cout << "\nWriter:" << endl;
  for( K3bDevice * dev = m_writer.first(); dev != 0; dev = m_writer.next() ) {
    cout << "  " << ": " << dev->devicename << " " << dev->vendor << " " 
	 << dev->description << " " << dev->version << " " << dev->maxWriteSpeed << endl;
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
  delete m_scsiIf;

  m_scsiIf = new ScsiIf( dev );

  if( m_scsiIf->init() != 0 ) {
    qDebug( "(K3bDeviceManager) Could not detect speed for device %s", dev );
    return 0;
  }

  unsigned char mp[32];

  if( getModePage( 0x2a, mp, 32, NULL, NULL, showErrorMsg ) != 0 ) {
    if( showErrorMsg ) {
      qDebug( "(K3bDeviceManager) Cannot retrieve drive capabilities mode page." );
    }
    return 0;
  }
  qDebug ( "(K3bDeviceManager) Get device information." );
  bool burnproof = ( mp[4] & 0x80 ) ? true : false;
  int accurateAudioStream = mp[5] & 0x02 ? 1 : 0;
  // speed must be diveded by 176
  int maxReadSpeed = ( mp[8] << 8 ) | mp[9];
  int currentReadSpeed = ( mp[14] << 8 ) | mp[15];
  currentReadSpeed = currentReadSpeed / 176;
  int maxWriteSpeed = ( mp[18] << 8 ) | mp[19];
  int currentWriteSpeed = ( mp[20] << 8 ) | mp[21];
  currentWriteSpeed = currentWriteSpeed / 176;
  bool burner = ( currentWriteSpeed > 0 ) ? true : false;

  K3bDevice* newDevice = new K3bDevice();
  newDevice->description = m_scsiIf->product();
  newDevice->vendor = m_scsiIf->vendor();
  newDevice->version = m_scsiIf->revision();
  newDevice->burner = burner;
  newDevice->burnproof = burnproof;
  newDevice->maxReadSpeed = currentReadSpeed;
  newDevice->devicename = dev;
  newDevice->maxWriteSpeed = currentWriteSpeed;

  return newDevice;
}


// Requests mode page 'pageCode' from device and places it into given
// buffer of maximum length 'bufLen'.
// modePageHeader: if != NULL filled with mode page header (8 bytes)
// blockDesc     : if != NULL filled with block descriptor (8 bytes),
//                 buffer is zeroed if no block descriptor is received
// return: 0: OK
//         1: scsi command failed
//         2: buffer too small for requested mode page
int K3bDeviceManager::getModePage( int pageCode, unsigned char *buf,
				   long bufLen, unsigned char *modePageHeader,
				   unsigned char *blockDesc, int showErrorMsg )
{
  unsigned char cmd[10];
  long dataLen = bufLen + 8 /*mode parameter header */ + 100 /*spare for block descriptors */ ;
  unsigned char *data = new ( unsigned char )[dataLen];

  memset( cmd, 0, 10 );
  memset( data, 0, dataLen );
  memset( buf, 0, bufLen );

  cmd[0] = 0x5a;      // MODE SENSE
  cmd[2] = pageCode & 0x3f;
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if( sendCmd( cmd, 10, NULL, 0, data, dataLen, showErrorMsg ) != 0 ) {
    delete[]data;
    return 1;
  }

  long modeDataLen = ( data[0] << 8 ) | data[1];
  long blockDescLen = ( data[6] << 8 ) | data[7];

  if( modePageHeader != NULL )
    memcpy( modePageHeader, data, 8 );

  if( blockDesc != NULL ) {
    if( blockDescLen >= 8 )
      memcpy( blockDesc, data + 8, 8 );
    else
      memset( blockDesc, 0, 8 );
  }

  if( modeDataLen > blockDescLen + 6 ) {
    unsigned char *modePage = data + blockDescLen + 8;
    long modePageLen = modePage[1] + 2;

    if( modePageLen > bufLen )
      modePageLen = bufLen;

    memcpy( buf, modePage, modePageLen );
    delete[]data;
    return 0;
  } else {
    qDebug( "No mode page data received." );
    delete[]data;
    return 1;
  }
}

// Sends SCSI command via 'scsiIf_'.
// return: see 'ScsiIf::sendCmd()'
int K3bDeviceManager::sendCmd( const unsigned char *cmd, int cmdLen,
			       const unsigned char *dataOut, int dataOutLen,
			       unsigned char *dataIn, int dataInLen,
			       int showErrorMsg )
{
  return m_scsiIf->sendCmd( cmd, cmdLen, dataOut, dataOutLen, dataIn,
			    dataInLen, showErrorMsg );
}
