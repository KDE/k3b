/***************************************************************************
                          k3bdevicechecker.cpp  -  description
                             -------------------
    begin                : Sun Sep 23 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <kapp.h>
#include <kprocess.h>
#include <kconfig.h>

#include "k3bdevicechecker.h"
#include "k3bscsibusid.h"
#include "k3bdevicemanager.h"
#include "device/ScsiIf.h"


K3bDeviceChecker::K3bDeviceChecker()
{
  m_scsiBusIds.setAutoDelete( true );
  scanbus();
  m_currentDevice = new K3bDevice();
}

K3bDeviceChecker::~K3bDeviceChecker()
{
  delete m_currentDevice;
  m_scsiBusIds.clear();
}

int K3bDeviceChecker::scanDevice( const char *dev, int showErrorMsg )
{
  m_scsiIf = new ScsiIf( dev );
  m_scsiIf->init();
  unsigned char mp[32];

  if( getModePage( 0x2a, mp, 32, NULL, NULL, showErrorMsg ) != 0 ) {
    if( showErrorMsg ) {
      qDebug( "Cannot retrieve drive capabilities mode page." );
    }
    return 1;
  }
  qDebug ( "Get device information." );
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
  K3bScsiBusId *id = getScsiIds( m_scsiIf->product() );
  m_currentDevice->bus = id->bus;
  m_currentDevice->target = id->target;
  m_currentDevice->lun = id->lun;
  m_currentDevice->description = m_scsiIf->product();
  m_currentDevice->vendor = m_scsiIf->vendor();
  m_currentDevice->version = m_scsiIf->revision();
  m_currentDevice->burner = burner;
  m_currentDevice->burnproof = burnproof;
  m_currentDevice->maxReadSpeed = currentReadSpeed;
  m_currentDevice->devicename = dev;
  m_currentDevice->maxWriteSpeed = currentWriteSpeed;
  return 0;
}

K3bScsiBusId *K3bDeviceChecker::getScsiIds( QString product )
{
  K3bScsiBusId *id;
  for( id = m_scsiBusIds.first(); id != 0; id = m_scsiBusIds.next() ) {
    if( ( id->product.compare( product ) ) == 0 ) {
      return id;
    }
  }
  return 0;
}

K3bDevice *K3bDeviceChecker::getCurrentDevice()
{
  return new K3bDevice( m_currentDevice );
  //return m_currentDevice;
}

int K3bDeviceChecker::scanbus()
{
  kapp->config()->setGroup( "External Programs" );

  // Use KProcess in block-mode while this should take less
  // than a second!
  KProcess *process = new KProcess();
  *process << kapp->config()->readEntry( "cdrecord path" );
  *process << "-scanbus";

  connect ( process, SIGNAL( receivedStdout( KProcess *, char *, int ) ),
	    this, SLOT( parseCdrecordOutput( KProcess *, char *, int ) ) );
  connect ( process, SIGNAL( receivedStderr( KProcess *, char *, int ) ),
	    this, SLOT( parseCdrecordOutput( KProcess *, char *, int ) ) );


  if( !process->start( KProcess::Block, KProcess::AllOutput ) )
    qDebug( "(K3bDeviceChecker) could not start cdrecord: %s",
	    kapp->config()->readEntry( "cdrecord path" ).latin1() );

  delete process;
  return 0;
}

void K3bDeviceChecker::parseCdrecordOutput( KProcess *, char *output, int len )
{
  QString buffer = QString::fromLatin1( output, len );

  // split to lines
  QStringList lines = QStringList::split( "\n", buffer );

  for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
    QString line( *str );
    line = line.stripWhiteSpace();
    if( line.startsWith( "0," )	&& line.at( line.length() - 1 ) != '*' ) {
      qDebug ( "parsing line: [[" + line + "]]" );
      // should be usable
      QString dev = line.mid( 0, 5 ).simplifyWhiteSpace();    // this should be OK
      uint _pos = line.find( ')' ) + 3;
      QString vendor = line.mid( _pos, 8 ).simplifyWhiteSpace();
      QString descr = line.mid( _pos + 11, 16 ).simplifyWhiteSpace();
      QString version = line.mid( _pos + 30, 4 ).simplifyWhiteSpace();

      // calculate id
      bool ok, ok2 = true;
      int bus = dev.mid( 0, 1 ).toInt( &ok );
      if( !ok )
	ok2 = false;
      int target = dev.mid( 2, 1 ).toInt( &ok );
      if( !ok )
	ok2 = false;
      int lun = dev.mid( 4, 1 ).toInt( &ok );
      if( !ok )
	ok2 = false;
      if( !ok2 )
	qDebug( "id-parsing did not work for: " + dev );

      m_scsiBusIds.append( new K3bScsiBusId( bus, target, lun, descr ) );
    }
  }   // for
}


// Requests mode page 'pageCode' from device and places it into given
// buffer of maximum length 'bufLen'.
// modePageHeader: if != NULL filled with mode page header (8 bytes)
// blockDesc     : if != NULL filled with block descriptor (8 bytes),
//                 buffer is zeroed if no block descriptor is received
// return: 0: OK
//         1: scsi command failed
//         2: buffer too small for requested mode page
int K3bDeviceChecker::getModePage( int pageCode, unsigned char *buf,
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
int K3bDeviceChecker::sendCmd( const unsigned char *cmd, int cmdLen,
			       const unsigned char *dataOut, int dataOutLen,
			       unsigned char *dataIn, int dataInLen,
			       int showErrorMsg )
{
  return m_scsiIf->sendCmd( cmd, cmdLen, dataOut, dataOutLen, dataIn,
			    dataInLen, showErrorMsg );
}
