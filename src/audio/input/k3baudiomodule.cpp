
#include "k3baudiomodule.h"
#include "../k3baudiotrack.h"

#include <qtimer.h>


K3bAudioModule::K3bAudioModule( K3bAudioTrack* track )
{
  m_track = track;
}


K3bAudioModule::~K3bAudioModule()
{
}


#include "k3baudiomodule.moc"
