
#include "k3baudiomodule.h"
#include "k3baudiotrack.h"

#include <qtimer.h>


K3bAudioModule::K3bAudioModule( K3bAudioTrack* track )
{
  m_track = track;
}


K3bAudioModule::~K3bAudioModule()
{
}


void K3bAudioModule::recalcLength()
{
  QTimer::singleShot( 0, this, SLOT(successFinish()) );
}


vois K3bAudioModule::successFinish()
{
  emit finished( true );
}



#include "k3baudiomodule.moc"
