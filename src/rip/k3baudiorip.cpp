#include "k3baudiorip.h"
#include "../device/k3bdevice.h"
#include "../tools/k3bcdparanoialib.h"

#include <qtimer.h>
#include <qfile.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>


// from cdda_paranoia.h
#define PARANOIA_CB_READ           0
#define PARANOIA_CB_VERIFY         1
#define PARANOIA_CB_FIXUP_EDGE     2
#define PARANOIA_CB_FIXUP_ATOM     3
#define PARANOIA_CB_SCRATCH        4
#define PARANOIA_CB_REPAIR         5
#define PARANOIA_CB_SKIP           6
#define PARANOIA_CB_DRIFT          7
#define PARANOIA_CB_BACKOFF        8
#define PARANOIA_CB_OVERLAP        9
#define PARANOIA_CB_FIXUP_DROPPED 10
#define PARANOIA_CB_FIXUP_DUPED   11
#define PARANOIA_CB_READERR       12


static K3bAudioRip* s_audioRip = 0;

void paranoiaCallback(long sector, int status)
{
  s_audioRip->createStatus(sector, status);
}



K3bAudioRip::K3bAudioRip( QObject* parent )
  : K3bJob( parent ),
    m_paranoiaLib(0),
    m_device(0),
    m_paranoiaMode(3),
    m_paranoiaRetries(20),
    m_neverSkip(false),
    m_track(1)
{
  m_rippingTimer = new QTimer( this );
  connect( m_rippingTimer, SIGNAL(timeout()), this, SLOT(slotParanoiaRead()) );
}


K3bAudioRip::~K3bAudioRip()
{
  delete m_paranoiaLib;
}


void K3bAudioRip::start()
{
  if( !m_paranoiaLib ) {
    m_paranoiaLib = K3bCdparanoiaLib::create();
  }

  if( !m_paranoiaLib ) {
    emit infoMessage( i18n("Could not load libcdparanoia."), ERROR );
    emit finished(false);
    return;
  }

  // try to open the device
  if( !m_device ) {
    emit finished(false);
    return;
  }

  if( !m_paranoiaLib->paranoiaInit( m_device->blockDeviceName() ) ) {
    emit infoMessage( i18n("Could not open device %1").arg(m_device->blockDeviceName()), ERROR );
    emit finished(false);
    return;
  }

  m_bInterrupt = m_bError = false;

  long firstSector = m_paranoiaLib->firstSector( m_track );
  m_lastSector = m_paranoiaLib->lastSector( m_track );
  
  if( firstSector < 0 || m_lastSector < 0 ) {
    emit finished(false);
    return;
  }

  // track length
  m_sectorsAll = m_lastSector - firstSector;
  m_sectorsRead = 0;
  
  m_paranoiaLib->setParanoiaMode( m_paranoiaMode );
  m_paranoiaLib->setNeverSkip( m_neverSkip );
  m_paranoiaLib->setMaxRetries( m_paranoiaRetries );

  m_paranoiaLib->paranoiaSeek( firstSector, SEEK_SET );
  m_currentSector = firstSector;

  m_rippingTimer->start(0);

  return;
}


void K3bAudioRip::slotParanoiaRead()
{
  if( m_bInterrupt){
    kdDebug() << "(K3bAudioRip) Interrupt reading." << endl;
    slotParanoiaFinished();
  } 
  else {
    // let the global paranoia callback have access to this
    // to emit signals
    s_audioRip = this;
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


void K3bAudioRip::createStatus( long sector, int status )
{
 
}

#include "k3baudiorip.moc"
