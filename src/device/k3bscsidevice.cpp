#include "k3bscsidevice.h"

#include "ScsiIf.h"

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
}

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "sg_err.h"


K3bScsiDevice::K3bScsiDevice( cdrom_drive* drive )
  : K3bDevice( drive )
{
  m_burnproof     = false;
  m_maxReadSpeed  = 0;
  m_maxWriteSpeed = 0;
  m_burner        = false;
}


K3bScsiDevice::~K3bScsiDevice()
{
}


bool K3bScsiDevice::init()
{
  qDebug( "(K3bScsiDevice) Initializing device %s...", genericDevice().latin1() );
  ScsiIf scsiIf( genericDevice().latin1() );
  if( scsiIf.init() != 0 ) {
    qDebug( "(K3bScsiDevice) Could not open device " + genericDevice() );
    return false;
  }


  // determine bus, target, lun
  int devFile = ::open( genericDevice().latin1(), O_RDONLY | O_NONBLOCK);
  if( !devFile ) {
    qDebug("(K3bScsiDevice) Could not open generic device.");
  }
  else {
    struct ScsiIdLun {
      int id;
      int lun;
    };
    int bus = -1;
    ScsiIdLun idLun;
    idLun.id = -1; idLun.lun = -1;

    if ( ioctl( devFile, SCSI_IOCTL_GET_BUS_NUMBER, &bus) < 0 ) {
      qDebug( "(K3bScsiDevice) %s: Need a filename that resolves to a SCSI device.", genericDevice().latin1() );
    }
    else {
      qDebug( "(K3bScsiDevice) bus: %i", bus );
      m_bus = bus;
    }
    if ( ioctl( devFile, SCSI_IOCTL_GET_IDLUN, &idLun ) < 0) {
      qDebug( "(K3bScsiDevice) %s: Need a filename that resolves to a SCSI device (2).", genericDevice().latin1() );
    }
    else {
      qDebug( "(K3bScsiDevice) id: %i lun: %i", idLun.id, idLun.lun );
      m_target = idLun.id;
      m_lun = idLun.lun;
    }
    ::close( devFile );
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

  qDebug( "(K3bScsiDevice) Setting device %s to maximum speed.", genericDevice().latin1() );
  if (scsiIf.sendCmd(cmd, 12, NULL, 0, NULL, 0, 0) != 0) {
    qDebug("(K3bScsiDevice) Cannot set device %s to maximum speed.", genericDevice().latin1() );
  }
  // -----------------------------------------------------------------------


  unsigned char mp[32];

  qDebug ( "(K3bScsiDevice) Get device information for device %s.", genericDevice().latin1() );
  if( getModePage( &scsiIf,  0x2a, mp, 32, NULL, NULL, 0 ) != 0 ) {
    qDebug( "(K3bScsiDevice) Cannot retrieve drive capabilities mode page from device %s.", genericDevice().latin1() );
    qDebug( "(K3bScsiDevice) Capabilities have to be set manually for device %s.", genericDevice().latin1() );
  }
  else {
    m_burnproof = ( mp[4] & 0x80 ) ? true : false;
    //  int accurateAudioStream = mp[5] & 0x02 ? 1 : 0;
    // speed must be diveded by 176
    m_maxReadSpeed = ( mp[14] << 8 ) | mp[15];
    m_maxReadSpeed  /= 176;
    m_maxWriteSpeed = ( mp[20] << 8 ) | mp[21];
    m_maxWriteSpeed /= 176;
    m_burner = ( m_maxWriteSpeed > 0 ) ? true : false;
    
    m_description = scsiIf.product();
    m_vendor = scsiIf.vendor();
    m_version = scsiIf.revision();
  }

  // testing
  // ---------------
  //  diskInfo();


  return true;
}



// checks if unit is ready
// return: 0: OK
//         1: scsi command failed
//         2: not ready
//         3: not ready, no disk in drive
//         4: not ready, tray out
int K3bScsiDevice::isReady() const
{
  ScsiIf scsiIf( genericDevice().latin1() );
  if( scsiIf.init() != 0 ) {
    qDebug( "(K3bScsiDevice) Could not open device " + genericDevice() );
    return 1;
  }

  unsigned char cmd[6];
  const unsigned char *sense;
  int senseLen;

  memset(cmd, 0, 6);

  switch( scsiIf.sendCmd(cmd, 6, NULL, 0, NULL, 0, 0) )
    {
    case 1:
      return 1;

    case 2:
      sense = scsiIf.getSense(senseLen);
    
      int code = sense[2] & 0x0f;
      
      if( code == 0x02 ) {
	// not ready
	return 2;
      }
      else if( code != 0x06 ) {
	scsiIf.printError();
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
// bool K3bScsiDevice::rezero()
// {
//   ScsiIf scsiIf( genericDevice().latin1() );
//   unsigned char cmd[6];
//   memset(cmd, 0, 6);
//   cmd[0] = 0x01;
  
//   if( scsiIf.sendCmd(cmd, 6, NULL, 0, NULL, 0, 0) != 0 ) {
//     qDebug( "Cannot rezero unit." );
//     return false;
//   }

//   return true;
// }


// Requests mode page 'pageCode' from device and places it into given
// buffer of maximum length 'bufLen'.
// modePageHeader: if != NULL filled with mode page header (8 bytes)
// blockDesc     : if != NULL filled with block descriptor (8 bytes),
//                 buffer is zeroed if no block descriptor is received
// return: 0: OK
//         1: scsi command failed
//         2: buffer too small for requested mode page
int K3bScsiDevice::getModePage( ScsiIf *_scsiIf, int pageCode, unsigned char *buf,
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
// bool K3bScsiDevice::cdCapacity( long* length )
// {
//   unsigned char cmd[10];
//   unsigned char data[8];

//   memset(cmd, 0, 10);
//   memset(data, 0, 8);

//   cmd[0] = 0x25; // READ CD-ROM CAPACITY

//   if( m_scsiIf->sendCmd(cmd, 10, NULL, 0, data, 8, 0) != 0 ) {
//     qDebug("(K3bScsiDevice) Cannot read capacity of device " + m_genericDevice );
//     return false;
//   }
  
//   *length = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
//   // *length += 1;

//   return true;
// }


int K3bScsiDevice::isEmpty()
{
  ScsiIf scsiIf( genericDevice().latin1() );
  if( scsiIf.init() != 0 ) {
    qDebug( "(K3bScsiDevice) Could not open device " + genericDevice() );
    return -1;
  }

  unsigned char cmd[10];
  unsigned long dataLen = 34;
  unsigned char data[34];

  // perform READ DISK INFORMATION
  memset(cmd, 0, 10);
  memset(data, 0, dataLen);

  cmd[0] = 0x51; // READ DISK INFORMATION
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if (scsiIf.sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) != 0) {
    qDebug( "(K3bScsiDevice) Could not check if disk in %s is empty.", genericDevice().latin1() );
    return -1;
  }

  return (data[2] & 0x03);
}


bool K3bScsiDevice::block( bool block ) const
{
  unsigned char cmd[6];

  memset(cmd, 0, 6);

  cmd[0] = 0x1e;
  
  if (block) {
    cmd[4] |= 0x01;
  }

  ScsiIf scsiIf( genericDevice().latin1() );
  if( scsiIf.init() != 0 ) {
    qDebug( "(K3bScsiDevice) Could not open device " + genericDevice() );
    return false;
  }

  if (scsiIf.sendCmd(cmd, 6, NULL, 0, NULL, 0) != 0) {
    qDebug( "(K3bScsiDevice) Cannot block/unblock device %s", genericDevice().latin1() );
    return false;
  }

  return true;
}


void K3bScsiDevice::diskInfo()
{
  ScsiIf  scsiIf( genericDevice().latin1() );
  if( scsiIf.init() != 0 ) {
    qDebug( "(K3bScsiDevice) Could not open device " + genericDevice() );
    return;
  }

  unsigned char cmd[10];
  unsigned long dataLen = 34;
  unsigned char data[34];
  char spd;

  // perform READ DISK INFORMATION
  memset(cmd, 0, 10);
  memset(data, 0, dataLen);

  cmd[0] = 0x51; // READ DISK INFORMATION
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if (scsiIf.sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) == 0) {
    qDebug( (data[2] & 0x10) ? "cdrw" : "cdr" );
    qDebug("capacity: %i %i %i", data[17], data[18], data[19]);

    switch (data[2] & 0x03) {
    case 0:
      // disc is empty
      qDebug("disc is empty");

      qDebug("capacity: %i %i %i", data[17], data[18], data[19]);
      break;

    case 1:
      // disc is not empty but appendable
      qDebug("number of sessions: %i", data[4]);
      qDebug("last track: %i", data[6]);

      qDebug("disk toc type: %i", data[8]);

      switch ((data[2] >> 2) & 0x03) {
      case 0:
	// last session is empty
	qDebug("is appendable");

	// don't count the empty session and invisible track
	qDebug("empty session");
	qDebug("some invisible track");

	break;

      case 1:
	// last session is incomplete (not fixated)
	// we cannot append in DAO mode, just update the statistic data
	
	qDebug("disk toc type: %i", data[8]);


	// don't count the invisible track
	qDebug("some invisible track");
	break;
      }
      break;

    case 2:
      // disk is complete
      qDebug("number of sessions: %i", data[4]);
      qDebug("last track: %i", data[6]);
      qDebug("disk toc type: %i", data[8]);
      break;
    }

    if (data[21] != 0xff || data[22] != 0xff || data[23] != 0xff) {
      qDebug("size: %i %i %i", data[21], data[22], data[23]);
    }
  }
  
  // perform READ TOC to get session info
  memset(cmd, 0, 10);
  dataLen = 12;
  memset(data, 0, dataLen);

  cmd[0] = 0x43; // READ TOC
  cmd[2] = 1; // get session info
  cmd[8] = dataLen; // allocation length

  if (scsiIf.sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) == 0) {
    int lastSessionLba = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];

    qDebug("last session lba??: %i", lastSessionLba );

    qDebug("another is empty: %s", ( (data[3] == 0) ? "yes" : "no" ));

    qDebug("another session number: %i", data[3]);
  }


  // read ATIP data
  dataLen = 28;
  memset(cmd, 0, 10);
  memset(data, 0, dataLen);

  cmd[0] = 0x43; // READ TOC/PMA/ATIP
  cmd[1] = 0x00;
  cmd[2] = 4; // get ATIP
  cmd[7] = 0;
  cmd[8] = dataLen; // data length
  
  if (scsiIf.sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) == 0) {
    if (data[6] & 0x04) {

      spd = (data[16] >> 4) & 0x07;
      qDebug("low speed: %i", (spd == 1 ? 2 : 0) );
      
      spd = (data[16] & 0x0f);
      qDebug("high speed: %i", ( spd >= 1 && spd <= 4 ? spd * 2 : 0) );
    }

    if (data[8] >= 80 && data[8] <= 99) {
      qDebug("anothar capacity: %i %i %i", data[8], data[9], data[10]);
    }
  }
}
