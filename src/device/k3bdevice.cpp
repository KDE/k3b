#include "k3bdevice.h"

#include "ScsiIf.h"



K3bDevice::K3bDevice( const char* devicename )
{
  m_devicename = devicename;
  m_scsiIf = new ScsiIf( devicename );
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
  m_scsiIf = new ScsiIf( m_devicename.latin1() );
}


K3bDevice::~K3bDevice()
{
  delete m_scsiIf;
}


bool K3bDevice::init()
{
  if( m_scsiIf->init() != 0 ) {
    qDebug( "(K3bDevice) Could not detect speed for device " + m_devicename );
    return false;
  }

  unsigned char mp[32];

  if( getModePage( 0x2a, mp, 32, NULL, NULL, 0 ) != 0 ) {
    qDebug( "(K3bDevice) Cannot retrieve drive capabilities mode page." );
    return false;
  }

  qDebug ( "(K3bDevice) Get device information for device " + m_devicename );

  m_burnproof = ( mp[4] & 0x80 ) ? true : false;
  int accurateAudioStream = mp[5] & 0x02 ? 1 : 0;
  // speed must be diveded by 176
  m_maxReadSpeed = ( mp[8] << 8 ) | mp[9];
  int currentReadSpeed = ( mp[14] << 8 ) | mp[15];
  currentReadSpeed  /= 176;
  m_maxWriteSpeed = ( mp[18] << 8 ) | mp[19];
  int currentWriteSpeed = ( mp[20] << 8 ) | mp[21];
  currentWriteSpeed /= 176;
  m_burner = ( currentWriteSpeed > 0 ) ? true : false;

  m_description = m_scsiIf->product();
  m_vendor = m_scsiIf->vendor();
  m_version = m_scsiIf->revision();


  // I do not really understand this, but
  // the max values are not usable!
  m_maxWriteSpeed = currentWriteSpeed;
  m_maxReadSpeed = currentReadSpeed;


  qDebug( "current write: %i, max write: %i", currentWriteSpeed, m_maxWriteSpeed );

  return true;
}



// checks if unit is ready
// return: 0: OK
//         1: scsi command failed
//         2: not ready
//         3: not ready, no disk in drive
//         4: not ready, tray out
int K3bDevice::isReady() const
{
  unsigned char cmd[6];
  const unsigned char *sense;
  int senseLen;

  memset(cmd, 0, 6);

  switch( m_scsiIf->sendCmd(cmd, 6, NULL, 0, NULL, 0, 0) )
    {
    case 1:
      return 1;

    case 2:
      sense = m_scsiIf->getSense(senseLen);
    
      int code = sense[2] & 0x0f;
      
      if( code == 0x02 ) {
	// not ready
	return 2;
      }
      else if( code != 0x06 ) {
	m_scsiIf->printError();
	return 1;
      }
      else {
	return 0;
      }
    }

  return 0;
}


// reset device to initial state
// return: 0: OK
//         1: scsi command failed
bool K3bDevice::rezero() const
{
  unsigned char cmd[6];

  memset(cmd, 0, 6);

  cmd[0] = 0x01;
  
  if( m_scsiIf->sendCmd(cmd, 6, NULL, 0, NULL, 0, 0) != 0 ) {
    qDebug( "Cannot rezero unit." );
    return false;
  }

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
int K3bDevice::getModePage( int pageCode, unsigned char *buf,
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

  if( m_scsiIf->sendCmd( cmd, 10, NULL, 0, data, dataLen, showErrorMsg ) != 0 ) {
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
bool K3bDevice::cdCapacity( long* length )
{
  unsigned char cmd[10];
  unsigned char data[8];

  memset(cmd, 0, 10);
  memset(data, 0, 8);

  cmd[0] = 0x25; // READ CD-ROM CAPACITY

  if( m_scsiIf->sendCmd(cmd, 10, NULL, 0, data, 8, 0) != 0 ) {
    qDebug("(K3bDevice) Cannot read capacity of device " + m_devicename );
    return false;
  }
  
  *length = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
  // *length += 1;

  return true;
}



// taken from cdrdao GenericMMC driver
// I think this should work for most of the cd drives out there
// ------------------------------------------------------------
// DiskInfo* K3bDevice::diskInfo()
// {
//   DiskInfo diskInfo_;
//   unsigned char cmd[10];
//   unsigned long dataLen = 34;
//   unsigned char data[34];
//   char spd;

//   memset(&diskInfo_, 0, sizeof(DiskInfo));

//   // perform READ DISK INFORMATION
//   memset(cmd, 0, 10);
//   memset(data, 0, dataLen);

//   cmd[0] = 0x51; // READ DISK INFORMATION
//   cmd[7] = dataLen >> 8;
//   cmd[8] = dataLen;

//   if (m_scsiIf->sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) == 0) {
//     diskInfo_.cdrw = (data[2] & 0x10) ? 1 : 0;
//     diskInfo_.valid.cdrw = 1;

//     switch (data[2] & 0x03) {
//     case 0:
//       // disc is empty
//       diskInfo_.empty = 1;
//       diskInfo_.append = 1;

//       diskInfo_.manufacturerId = Msf(data[17], data[18], data[19]);
//       diskInfo_.valid.manufacturerId = 1;
//       break;

//     case 1:
//       // disc is not empty but appendable
//       diskInfo_.sessionCnt = data[4];
//       diskInfo_.lastTrackNr = data[6];

//       diskInfo_.diskTocType = data[8];

//       switch ((data[2] >> 2) & 0x03) {
//       case 0:
// 	// last session is empty
// 	diskInfo_.append = 1;

// 	// don't count the empty session and invisible track
// 	diskInfo_.sessionCnt -= 1;
// 	diskInfo_.lastTrackNr -= 1;
// 	break;

//       case 1:
// 	// last session is incomplete (not fixated)
// 	// we cannot append in DAO mode, just update the statistic data
	
// 	diskInfo_.diskTocType = data[8];

// 	// don't count the invisible track
// 	diskInfo_.lastTrackNr -= 1;
// 	break;
//       }
//       break;

//     case 2:
//       // disk is complete
//       diskInfo_.sessionCnt = data[4];
//       diskInfo_.lastTrackNr = data[6];
//       diskInfo_.diskTocType = data[8];
//       break;
//     }

//     diskInfo_.valid.empty = 1;
//     diskInfo_.valid.append = 1;

//     if (data[21] != 0xff || data[22] != 0xff || data[23] != 0xff) {
//       diskInfo_.valid.capacity = 1;
//       diskInfo_.capacity = Msf(data[21], data[22], data[23]).lba() - 150;
//     }
//   }

//   // perform READ TOC to get session info
//   memset(cmd, 0, 10);
//   dataLen = 12;
//   memset(data, 0, dataLen);

//   cmd[0] = 0x43; // READ TOC
//   cmd[2] = 1; // get session info
//   cmd[8] = dataLen; // allocation length

//   if (m_scsiIf->sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) == 0) {
//     diskInfo_.lastSessionLba = (data[8] << 24) | (data[9] << 16) |
//                                (data[10] << 8) | data[11];

//     if (!diskInfo_.valid.empty) {
//       diskInfo_.valid.empty = 1;
//       diskInfo_.empty = (data[3] == 0) ? 1 : 0;

//       diskInfo_.sessionCnt = data[3];
//     }
//   }

//   // read ATIP data
//   dataLen = 28;
//   memset(cmd, 0, 10);
//   memset(data, 0, dataLen);

//   cmd[0] = 0x43; // READ TOC/PMA/ATIP
//   cmd[1] = 0x00;
//   cmd[2] = 4; // get ATIP
//   cmd[7] = 0;
//   cmd[8] = dataLen; // data length
  
//   if (m_scsiIf->sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) == 0) {
//     if (data[6] & 0x04) {
//       diskInfo_.valid.recSpeed = 1;
//       spd = (data[16] >> 4) & 0x07;
//       diskInfo_.recSpeedLow = spd == 1 ? 2 : 0;
      
//       spd = (data[16] & 0x0f);
//       diskInfo_.recSpeedHigh = spd >= 1 && spd <= 4 ? spd * 2 : 0;
//     }

//     if (data[8] >= 80 && data[8] <= 99) {
//       diskInfo_.manufacturerId = Msf(data[8], data[9], data[10]);
//       diskInfo_.valid.manufacturerId = 1;
//     }
//   }

//   return &diskInfo_;
// }
