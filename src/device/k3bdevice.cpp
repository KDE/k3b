#include "k3bdevice.h"

#include "ScsiIf.h"



K3bDevice::K3bDevice( const char* devicename )
{
  m_devicename = devicename;
  //m_scsiIf = new ScsiIf( devicename );
}


K3bDevice::K3bDevice( const QString & _vendor,
		       const QString & _description,
		       const QString & _version,
		       bool _burner,
		       bool _burnproof,
		       int _maxReadSpeed,
		       const QString & _devicename, int _maxBurnSpeed = 0 )
  : m_vendor( _vendor ),
    m_description( _description ), m_version( _version ), m_burner( _burner ),
    m_burnproof( _burnproof ), m_maxReadSpeed( _maxReadSpeed ),
    m_devicename( _devicename ), m_maxWriteSpeed( _maxBurnSpeed ) 
{
  //m_scsiIf = new ScsiIf( m_devicename.latin1() );
}


K3bDevice::~K3bDevice()
{
  //delete m_scsiIf;
}


bool K3bDevice::init()
{
  qDebug( "(K3bDevice) Initializing device %s...", m_devicename.latin1() );
  ScsiIf  *_scsiIf = new ScsiIf( m_devicename.latin1() );
  if( _scsiIf->init() != 0 ) {
    qDebug( "(K3bDevice) Could not open device " + m_devicename );
    delete _scsiIf;
    return false;
  }



  // taken from cdrdao's GenericMMC driver
  // this should (hopefully) work with most cd-drives
  // -----------------------------------------------------------------------
  unsigned char cmd[12];
  memset(cmd, 0, 12);

  cmd[0] = 0xbb; // SET CD SPEED
  cmd[2] = 0xff; // select maximum read speed
  cmd[3] = 0xff;
  cmd[4] = 0xff; // select maximum write speed
  cmd[5] = 0xff;

  qDebug( "(K3bDevice) Setting device %s to maximum speed.", m_devicename.latin1() );
  if (_scsiIf->sendCmd(cmd, 12, NULL, 0, NULL, 0, 0) != 0) {
    qDebug("(K3bDevice) Cannot set device %s to maximum speed.", m_devicename.latin1() );
  }
  // -----------------------------------------------------------------------


  unsigned char mp[32];

  qDebug ( "(K3bDevice) Get device information for device %s.", m_devicename.latin1() );
  if( getModePage( _scsiIf,  0x2a, mp, 32, NULL, NULL, 0 ) != 0 ) {
    qDebug( "(K3bDevice) Cannot retrieve drive capabilities mode page from device %s.", m_devicename.latin1() );
    delete _scsiIf;
    return false;
  }

  m_burnproof = ( mp[4] & 0x80 ) ? true : false;
  //  int accurateAudioStream = mp[5] & 0x02 ? 1 : 0;
  // speed must be diveded by 176
  m_maxReadSpeed = ( mp[14] << 8 ) | mp[15];
  m_maxReadSpeed  /= 176;
  m_maxWriteSpeed = ( mp[20] << 8 ) | mp[21];
  m_maxWriteSpeed /= 176;
  m_burner = ( m_maxWriteSpeed > 0 ) ? true : false;

  m_description = _scsiIf->product();
  m_vendor = _scsiIf->vendor();
  m_version = _scsiIf->revision();


  qDebug( "(K3bDevice) %s max write: %i", m_devicename.latin1(), m_maxWriteSpeed );
  qDebug( "(K3bDevice) %s max read : %i", m_devicename.latin1(), m_maxReadSpeed );
  qDebug("");
  delete _scsiIf;
  return true;
}



// checks if unit is ready
// return: 0: OK
//         1: scsi command failed
//         2: not ready
//         3: not ready, no disk in drive
//         4: not ready, tray out
int K3bDevice::isReady()
{
  ScsiIf *_scsiIf = new ScsiIf( m_devicename.latin1() );
  unsigned char cmd[6];
  const unsigned char *sense;
  int senseLen;

  memset(cmd, 0, 6);

  switch( _scsiIf->sendCmd(cmd, 6, NULL, 0, NULL, 0, 0) )
    {
    case 1:
      return 1;

    case 2:
      sense = _scsiIf->getSense(senseLen);
    
      int code = sense[2] & 0x0f;
      
      if( code == 0x02 ) {
	// not ready
	delete _scsiIf;
	return 2;
      }
      else if( code != 0x06 ) {
	_scsiIf->printError();
	delete _scsiIf;
	return 1;
      }
      else {
	delete _scsiIf;
	return 0;
      }
    }
  delete _scsiIf;
  return 0;
}


// reset device to initial state
// return: 0: OK
//         1: scsi command failed
bool K3bDevice::rezero()
{
  ScsiIf *_scsiIf = new ScsiIf( m_devicename.latin1() );
  unsigned char cmd[6];
  memset(cmd, 0, 6);
  cmd[0] = 0x01;
  
  if( _scsiIf->sendCmd(cmd, 6, NULL, 0, NULL, 0, 0) != 0 ) {
    qDebug( "Cannot rezero unit." );
    delete _scsiIf;
    return false;
  }

  delete _scsiIf;
  return true;
}


// Requests mode page 'pageCode' from device and places it into given
// buffer of maximum length 'bufLen'.
// modePageHeader: if != NULL filled with mode page header (8 bytes)
// blockDesc     : if != NULL filled with block descriptor (8 bytes),
//                 buffer is zeroed if no block descriptor is received
// return: 0: OK
//         1: scsi command failed
//         2: buffer too small for requested mode page
int K3bDevice::getModePage( ScsiIf *_scsiIf, int pageCode, unsigned char *buf,
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

  if( _scsiIf->sendCmd( cmd, 10, NULL, 0, data, dataLen, showErrorMsg ) != 0 ) {
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


// Reads the cd-rom capacity and stores the total number of available blocks
// in 'length'.
// return: 0: OK
//         1: SCSI command failed
// bool K3bDevice::cdCapacity( long* length )
// {
//   unsigned char cmd[10];
//   unsigned char data[8];

//   memset(cmd, 0, 10);
//   memset(data, 0, 8);

//   cmd[0] = 0x25; // READ CD-ROM CAPACITY

//   if( m_scsiIf->sendCmd(cmd, 10, NULL, 0, data, 8, 0) != 0 ) {
//     qDebug("(K3bDevice) Cannot read capacity of device " + m_devicename );
//     return false;
//   }
  
//   *length = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
//   // *length += 1;

//   return true;
// }


int K3bDevice::isEmpty()
{
  ScsiIf *_scsiIf = new ScsiIf( m_devicename.latin1() );
  unsigned char cmd[10];
  unsigned long dataLen = 34;
  unsigned char data[34];

  // perform READ DISK INFORMATION
  memset(cmd, 0, 10);
  memset(data, 0, dataLen);

  cmd[0] = 0x51; // READ DISK INFORMATION
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if (_scsiIf->sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) != 0) {
    qDebug( "(K3bDevice) Could not check if disk in %s is empty.", m_devicename.latin1() );
    delete _scsiIf;
    return -1;
  }
  delete _scsiIf;
  return (data[2] & 0x03);
}


bool K3bDevice::block( bool block ) const
{
  unsigned char cmd[6];

  memset(cmd, 0, 6);

  cmd[0] = 0x1e;
  
  if (block) {
    cmd[4] |= 0x01;
  }

  ScsiIf *scsiIf = new ScsiIf( m_devicename.latin1() );
  if (scsiIf->sendCmd(cmd, 6, NULL, 0, NULL, 0) != 0) {
    qDebug( "(K3bDevice) Cannot block/unblock device %s", devicename.latin1() );
    delete scsiIf;
    return false;
  }

  delete scsiIf;

  return true;
}
