#include "k3bdiskinfodetector.h"

#include "../device/k3bdevice.h"
#include "../device/k3btoc.h"
#include "../rip/k3btcwrapper.h"

#include <qtimer.h>

extern "C" {
#include <cdda_interface.h>
}



K3bDiskInfoDetector* K3bDiskInfoDetector::detect( K3bDevice* dev )
{
  K3bDiskInfoDetector* d = new K3bDiskInfoDetector( dev );
  QTimer::singleShot( 0, d, SLOT(slotDetect()) );

  return d;
}


K3bDiskInfoDetector::K3bDiskInfoDetector( K3bDevice* device )
  : QObject(), m_device(device)
{
}


void K3bDiskInfoDetector::slotDetect()
{
  // TODO: it would be great to have this async! But I think that's only possible 
  //       with threading!

  m_info.device = m_device;


  switch( m_device->isEmpty() ) {
  case 0:
    m_info.empty = true;
    break;
  case 1:
    m_info.empty = false;
    m_info.appendable = true;
    break;
  case 2:
    m_info.empty = false;
    m_info.appendable = false;
    break;
  default:
    m_info.noDisk = true;
  }

  cdrom_drive* drive = m_device->open();
  if( !drive ) {
    m_info.noDisk = true;
  }
  else {
    // retrieve TOC
    int discFirstSector = cdda_disc_firstsector( drive );
    m_info.toc.setFirstSector( discFirstSector );

    int nAudio = 0, nData = 0;    
    int tracks = cdda_tracks( drive );
    for( int i = 1; i <= tracks; i++ ) {
      
      int firstSector = cdda_track_firstsector( drive, i );
      int lastSector = cdda_track_lastsector( drive, i );
      int type = ( cdda_track_audiop( drive, i ) ? K3bTrack::AUDIO : K3bTrack::DATA );
      type == K3bTrack::AUDIO ? nAudio++ : nData++;

      m_info.toc.append( new K3bTrack(firstSector, lastSector, type, QString("Track %1").arg(i)) );
    }

    m_device->close();

    if( nAudio > 0 )
      m_info.tocType = K3bDiskInfo::AUDIO;
    if( nData > 0 )
      m_info.tocType = K3bDiskInfo::DATA;
    if( nAudio > 0 && nData > 0 )
      m_info.tocType = K3bDiskInfo::MIXED;

    m_info.empty = ( nAudio + nData == 0 );
  }


  if( K3bTcWrapper::supportDvd() ) {
    // check if it is a dvd we can display

    m_tcWrapper = new K3bTcWrapper( this );
    connect( m_tcWrapper, SIGNAL(successfulDvdCheck(bool)), this, SLOT(slotIsDvd(bool)) );
    m_tcWrapper->checkDvd( m_device );
  }
  else {
    emit diskInfoReady( m_info );

    QTimer::singleShot( 0, this, SLOT(delayedDestruct()) );
  }
}


void K3bDiskInfoDetector::slotIsDvd( bool dvd )
{
  if( dvd ) {
    m_info.empty = false;
    m_info.noDisk = false;
    m_info.tocType = K3bDiskInfo::DVD;
  }

  emit diskInfoReady( m_info );

  QTimer::singleShot( 0, this, SLOT(delayedDestruct()) );
}


void K3bDiskInfoDetector::delayedDestruct()
{
  delete this;
}


#include "k3bdiskinfodetector.moc"
