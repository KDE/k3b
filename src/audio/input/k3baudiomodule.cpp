
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


void K3bAudioModule::setConsumer( QObject* c = 0, const char* goOnSignal = 0 )
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
