
#include "k3baudiomodule.h"
#include "../k3baudiotrack.h"

#include <qtimer.h>


K3bAudioModule::K3bAudioModule( K3bAudioTrack* track )
{
  m_track = track;
  m_consumer = 0;
}


K3bAudioModule::~K3bAudioModule()
{
}


void K3bAudioModule::start()
{
  QTimer::singleShot( 0, this, SLOT(startDecoding()) );
}


void K3bAudioModule::setConsumer( QObject* c, const char* goOnSignal )
{
  if( m_consumer ) {
    disconnect(m_consumer);
    m_consumer = 0;
  }

  if( c  && goOnSignal != 0 ) {
    m_consumer = c;
    connect( m_consumer, goOnSignal, this, SLOT(slotConsumerReady()) );
  }
}


#include "k3baudiomodule.moc"
