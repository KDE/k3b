#include "k3baudiorip.h"
#include "../device/k3bdevice.h"
#include "../tools/k3bcdparanoialib.h"

#include <qtimer.h>
#include <qfile.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>


void paranoiaCallback(long, int){
  // Do we want to show info somewhere ?
  // Not yet.
}


K3bAudioRip::K3bAudioRip( QObject* parent )
  : QObject( parent ),
    m_paranoiaLib(0),
    m_paranoiaMode(3),
    m_paranoiaRetries(20)
{
  m_rippingTimer = new QTimer( this );
  connect( m_rippingTimer, SIGNAL(timeout()), this, SLOT(slotParanoiaRead()) );
}


K3bAudioRip::~K3bAudioRip()
{
  delete m_paranoiaLib;
}


bool K3bAudioRip::ripTrack( K3bDevice* dev, unsigned int track )
{
  if( !m_paranoiaLib )
    m_paranoiaLib = K3bCdparanoiaLib::create();

  if( !m_paranoiaLib ) {
    KMessageBox::error( 0, i18n("Could not load libcd_paranoia. Please install.") );
    return false;
  }

  m_device = dev;

  // try to open the device
  if( !dev )
    return false;

  if( !m_paranoiaLib->paranoiaInit( dev->blockDeviceName() ) ) {
    kdDebug() << "(K3bAudioRip) Could not open device" << endl;
    return false;
  }

  m_bInterrupt = m_bError = false;

  long firstSector = m_paranoiaLib->firstSector( track );
  m_lastSector = m_paranoiaLib->lastSector( track );
  
  if( firstSector < 0 || m_lastSector < 0 )
    return false;

  // track length
  m_sectorsAll = m_lastSector - firstSector;
  m_sectorsRead = 0;
  
  m_paranoiaLib->setParanoiaMode( m_paranoiaMode );
  m_paranoiaLib->setMaxRetries( m_paranoiaRetries );

  m_paranoiaLib->paranoiaSeek( firstSector, SEEK_SET );
  m_currentSector = firstSector;

  m_rippingTimer->start(0);

  return true;
}


void K3bAudioRip::slotParanoiaRead()
{
  if( m_bInterrupt){
    kdDebug() << "(K3bAudioRip) Interrupt reading." << endl;
    slotParanoiaFinished();
  } 
  else {
    int16_t* buf = m_paranoiaLib->paranoiaRead( paranoiaCallback );

    if( 0 == buf ) {
      kdDebug() << "(K3bAudioRip) Unrecoverable error in paranoia_read" << endl;
      m_bError = true;
      slotParanoiaFinished();
    } 
    else {
      ++m_currentSector;
      QByteArray a;
      a.setRawData( (char*)buf, CD_FRAMESIZE_RAW );
      emit output( a );
      a.resetRawData( (char*)buf, CD_FRAMESIZE_RAW );

      ++m_sectorsRead;
      emit percent( 100 * m_sectorsRead / m_sectorsAll );
      
      if ( m_currentSector >= m_lastSector ) {
	slotParanoiaFinished();
      }
    }
  }
}


void K3bAudioRip::slotParanoiaFinished()
{
  m_rippingTimer->stop();
  m_paranoiaLib->paranoiaFree();

  if( m_bInterrupt || m_bError )
    emit finished( false );
  else
    emit finished( true );
}


void K3bAudioRip::cancel()
{
  m_bInterrupt = true;
}


#include "k3baudiorip.moc"
