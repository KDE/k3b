#include "k3bscsidevice.h"

#include "ScsiIf.h"

typedef Q_INT16 size16;
typedef Q_INT32 size32;


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "sg_err.h"
#include <kdebug.h>
#include <qfile.h>


K3bScsiDevice::K3bScsiDevice( const QString& devname )
  : K3bDevice( devname )
{
  m_burnproof     = false;
  m_maxReadSpeed  = 1;
  m_maxWriteSpeed = 1;
  m_burner        = false;
}


K3bScsiDevice::~K3bScsiDevice()
{
}


// checks if unit is ready
// return: 0: OK
//         1: scsi command failed
//         2: not ready
//         3: not ready, no disk in drive
//         4: not ready, tray out
int K3bScsiDevice::isReady() const
{
  ScsiIf scsiIf( QFile::encodeName(genericDevice()) );
  if( scsiIf.init() != 0 ) {
    kdDebug() << "(K3bScsiDevice) Could not open device " << genericDevice() << endl;
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


bool K3bScsiDevice::rewritable()
{
  ScsiIf scsiIf( QFile::encodeName(genericDevice()) );
  if( scsiIf.init() != 0 ) {
    kdDebug() << "(K3bScsiDevice) Could not open device " << genericDevice() << endl;
    return false;
  }

  unsigned char cmd[10];
  unsigned long dataLen = 34;
  unsigned char data[34];

  memset(cmd, 0, 10);
  memset(data, 0, dataLen);

  cmd[0] = 0x51; // READ DISK INFORMATION
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if( scsiIf.sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) ) {
    kdDebug() << "(K3bScsiDevice) scsi command failed." << endl;
    return false;
  }

  return (data[2] & 0x10);
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
//     kdDebug() << "Cannot rezero unit." << endl;
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
    kdDebug() << "No mode page data received." << endl;
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
//     kdDebug() << "(K3bScsiDevice) Cannot read capacity of device " << m_genericDevice << endl;
//     return false;
//   }
  
//   *length = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
//   // *length += 1;

//   return true;
// }


int K3bScsiDevice::isEmpty()
{
  ScsiIf scsiIf( QFile::encodeName(genericDevice()) );
  if( scsiIf.init() != 0 ) {
    kdDebug() << "(K3bScsiDevice) Could not open device " << genericDevice() << endl;
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
    kdDebug() << "(K3bScsiDevice) Could not check if disk in " << genericDevice() << " is empty." << endl;
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

  ScsiIf scsiIf( QFile::encodeName(genericDevice()) );
  if( scsiIf.init() != 0 ) {
    kdDebug() << "(K3bScsiDevice) Could not open device " << genericDevice() << endl;
    return false;
  }

  if (scsiIf.sendCmd(cmd, 6, NULL, 0, NULL, 0) != 0) {
    kdDebug() << "(K3bScsiDevice) Cannot block/unblock device " << genericDevice() << endl;
    return false;
  }

  return true;
}
