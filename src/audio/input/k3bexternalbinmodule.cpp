#include "k3bexternalbinmodule.h"
#include "../k3baudiotrack.h"

#include <kurl.h>
#include <kapp.h>
#include <kconfig.h>
#include <kprocess.h>
#include <klocale.h>

#include <qstring.h>
#include <qtimer.h>

#include <kprocess.h>

#include <cmath>
#include <iostream>



K3bExternalBinModule::K3bExternalBinModule( K3bAudioTrack* track )
  : K3bAudioModule( track )
{
  m_infoTimer = new QTimer( this );
  m_clearDataTimer = new QTimer( this );
  m_process = new KShellProcess();


  connect( m_infoTimer, SIGNAL(timeout()), this, SLOT(slotGatherInformation()) );
  connect( m_clearDataTimer, SIGNAL(timeout()), this, SLOT(slotClearData()) );


  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedStdout(KProcess*,char*, int)) );
  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)), 
	   this, SLOT(slotParseStdErrOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotProcessFinished()) );


  m_rawDataSoFar = 0;
  m_currentData = 0;
  m_currentDataLength = 0;
  m_finished = true;

  m_infoTimer->start(0, true);


  // testing
//   m_testFile = new QFile( "/home/trueg/download/test_k3b_module.cd" );
//   m_testFile->open( IO_WriteOnly );
  //-----------------
}


K3bExternalBinModule::~K3bExternalBinModule()
{
  delete m_process;
  delete [] m_currentData;
}


bool K3bExternalBinModule::getStream()
{
  if( m_process->isRunning() ) {
    m_process->kill();
  }

  m_process->clearArguments();


  addArguments();


  if( !m_process->start( KProcess::NotifyOnExit, KProcess::Stdout ) ) {
    qDebug( "(K3bExternalBinModule) could not start process." );
    return false;
  }
  else {
    qDebug( "(K3bExternalBinModule) Started process." );
    m_rawDataSoFar = 0;
    m_currentDataLength = 0;
    m_currentData = 0;
    m_finished = false;
    return true;
  }
}


void K3bExternalBinModule::slotReceivedStdout( KProcess*, char* data, int len )
{
  // we do not take data anymore if the finished flag is true
  if( !m_finished ) {
    if( m_rawDataSoFar + len > audioTrack()->length() * 2352 ) {
      // something went wrong with the track's size and we need
      // to cut the track off. This is bad but it must be done in order
      // to create a correctly working CD!

      qDebug( "(K3bExternalBinModule) Got more data as expected. Cut data at track length!" );
      qDebug( "(K3bExternalBinModule) This could cause the track to be cut off but it will" );
      qDebug( "(K3bExternalBinModule) assure a correct Audio CD!    Sorry!" );

      len = audioTrack()->length() * 2352 - m_rawDataSoFar;

      if( len < 0 ) {
	qDebug( "(K3bExternalBinModule) Len < 0" );
	len = 0;
      }

      m_finished = true;
      m_process->closeStdout();
      m_process->closeStderr();
      m_process->kill();
    }


    if( m_currentDataLength != 0 ) {
      qDebug( "(K3bExternalBinModule) received stdout and current data was not 0!" );
      qDebug( "(K3bExternalBinModule) also this is NOT supposed to happen, we will work around it! ;-)" );

      // only expand the current data
      char* buffer = m_currentData;
      m_currentData = new char[m_currentDataLength + len];
      memcpy( m_currentData, buffer, m_currentDataLength );
      memcpy( &m_currentData[m_currentDataLength], data, len );
      delete [] buffer;

      m_currentDataLength += len;
    }
    else {
      m_currentData = new char[len];
      m_currentDataLength = len;
      memcpy( m_currentData, data, len );

      emit output(len);
    }

    m_rawDataSoFar += len;

    m_process->suspend();

    // testing
    //   m_testFile->writeBlock( data, len );
    //   m_testFile->flush();
    //----------------
  }
}


void K3bExternalBinModule::slotProcessFinished()
{
  if( !m_finished ) {

    qDebug( "(K3bExternalBinModule) process finished. padding to multible of 2352 bytes!" );

    m_finished = true;

    // mpg123 does not ensure it's output to be a multible of blocks (2352 bytes)
    // since we round up the number of frames in slotCountRawDataFinished
    // we need to pad by ourself to ensure cdrdao splits our data stream
    // correcty
    int diff = m_rawDataSoFar % 2352;
    int bytesToPad = 2352 - diff;
  
    if( bytesToPad > 0 )
      {
	if( m_currentDataLength == 0 ) {
	  m_currentData = new char[bytesToPad];
	  m_currentDataLength = bytesToPad;
	  memset( m_currentData, bytesToPad, 0 );

	  emit output( bytesToPad );
	}
	else {
	  // only expand the current data
	  char* buffer = m_currentData;
	  m_currentData = new char[m_currentDataLength + bytesToPad];
	  memcpy( m_currentData, buffer, m_currentDataLength );
	  memset( &m_currentData[m_currentDataLength], bytesToPad, 0 );
	  delete [] buffer;

	  m_currentDataLength += bytesToPad;

	  // no need to emit output signal since we still waint for data to be read
	}

	// testing
	//       m_testFile->writeBlock( m_currentData, bytesToPad );
	//       m_testFile->flush();
	//----------------

      }

    if( m_rawDataSoFar + bytesToPad < audioTrack()->length() * 2352 ) {
      // something went wrong with the track's size. It is too big
      // So we need to pad the missing bytes with 0
      qDebug( "(K3bExternalBinModule) Process did not send enough data. Padding with NULL!" );
      qDebug( "(K3bExternalBinModule) This could be caused by wrong track size calculation!" );
      qDebug( "(K3bExternalBinModule) It will NOT corrupt your CD but only create a little silence after the current track!" );

      bytesToPad = audioTrack()->length() * 2352 - m_rawDataSoFar - bytesToPad;

      char* buffer = m_currentData;
      m_currentData = new char[m_currentDataLength + bytesToPad];
      memcpy( m_currentData, buffer, m_currentDataLength );
      memset( &m_currentData[m_currentDataLength], bytesToPad, 0 );
      delete [] buffer;

      m_currentDataLength += bytesToPad;
    }
  
  }
}


int K3bExternalBinModule::readData( char* data, int len )
{
  m_clearDataTimer->start(0, true);

  if( m_currentDataLength == 0 ) {
    return 0;
  }
  else if( len >= m_currentDataLength ) {
    int i = m_currentDataLength;
    m_currentDataLength = 0;

    memcpy( data, m_currentData, i );

    delete [] m_currentData;
    m_currentData = 0;
    m_process->resume();

    return i;
  }
  else {
    qDebug("(K3bExternalBinModule) readData: %i bytes available but only %i requested!", 
	   m_currentDataLength, len );

    memcpy( data, m_currentData, len );

    char* buffer = m_currentData;
    m_currentData = new char[m_currentDataLength - len];
    memcpy( m_currentData, &buffer[len], m_currentDataLength - len );
    delete [] buffer;

    m_currentDataLength -= len;

    return len;
  }
}


void K3bExternalBinModule::slotClearData()
{
  if( m_finished && m_currentDataLength == 0 ) {
//     m_testFile->close();
    emit finished( true );
  }

  else if( m_currentDataLength > 0 )
    emit output( m_currentDataLength );

  m_clearDataTimer->stop();
}


void K3bExternalBinModule::cancel()
{
  m_finished = true;

  // cancel whatever in process
  m_process->closeStdin();
  m_process->closeStderr();
  m_process->kill();

  m_clearDataTimer->stop();
  m_infoTimer->stop();

  delete [] m_currentData;
  m_currentDataLength = 0;

  emit finished( false );
}


#include "k3bexternalbinmodule.moc"
