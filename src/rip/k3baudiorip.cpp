#include "k3baudiorip.h"
#include "../device/k3bdevice.h"

#include <qtimer.h>
#include <qfile.h>

#include <kdebug.h>



void paranoiaCallback(long, int){
  // Do we want to show info somewhere ?
  // Not yet.
}


K3bAudioRip::K3bAudioRip( QObject* parent )
  : QObject( parent ),
    m_cdromDrive( 0 )
{
  m_rippingTimer = new QTimer( this );
  connect( m_rippingTimer, SIGNAL(timeout()), this, SLOT(slotParanoiaRead()) );
  m_paranoia = 0;
  m_device = 0;
}


K3bAudioRip::~K3bAudioRip()
{
}


bool K3bAudioRip::ripTrack( K3bDevice* dev, unsigned int track )
{
  m_device = dev;

  // try to open the device
  if( !dev )
    return false;

  if( !open() ) {
    kdDebug() << "(K3bAudioRip) Could not open device" << endl;
    return false;
  }

  m_bInterrupt = m_bError = false;

  long firstSector = cdda_track_firstsector( open(), track );
  m_lastSector = cdda_track_lastsector( open(), track );

  if( firstSector < 0 || m_lastSector < 0 )
    return false;

  // track length
  m_sectorsAll = m_lastSector - firstSector;
  m_sectorsRead = 0;
  
  kdDebug() << "(K3bAudioRip) paranoia_init" << endl;
  m_paranoia = paranoia_init( open() );
  
  if( 0 == m_paranoia ) {
    kdDebug() << "(K3bAudioRip) paranoia_init failed" << endl;
    return false;
  }
  
  int paranoiaLevel = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;
  paranoia_modeset( m_paranoia, paranoiaLevel );

  cdda_verbose_set( open(), CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT );

  paranoia_seek( m_paranoia, firstSector, SEEK_SET );
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
    int16_t* buf = paranoia_read( m_paranoia, paranoiaCallback );

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
  paranoia_free(m_paranoia);
  close();

  if( m_bInterrupt || m_bError )
    emit finished( false );
  else
    emit finished( true );
}


void K3bAudioRip::cancel()
{
  m_bInterrupt = true;
}


cdrom_drive* K3bAudioRip::open()
{
  if( m_cdromDrive == 0 ) {
    m_cdromDrive = cdda_identify( QFile::encodeName(m_device->devicename()), CDDA_MESSAGE_FORGETIT, 0 );
    if( !m_cdromDrive ) {
      kdDebug() << "(K3bAudioRip) Could not open device " << m_device->devicename() << endl;
      return 0;
    }
    cdda_open( m_cdromDrive );
    return m_cdromDrive;
  }
  else
    return m_cdromDrive;
}


bool K3bAudioRip::close()
{
  if( m_cdromDrive == 0 )
    return false;
  else {
    cdda_close( m_cdromDrive );
    m_cdromDrive = 0;
    return true;
  }
}

#include "k3baudiorip.moc"
