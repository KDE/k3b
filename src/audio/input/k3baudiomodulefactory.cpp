
#include "k3baudiomodulefactory.h"
#include "k3bmp3module.h"
#include "k3bwavmodule.h"
#include "../k3baudiotrack.h"


#include <qstring.h>
#include <qfileinfo.h>

#include <kurl.h>



K3bAudioModule* K3bAudioModuleFactory::createModule( K3bAudioTrack* track )
{
  QString _extension = QFileInfo( track->fileName() ).extension(false);

  // ==============
  // for now we do no deep filetype-checking but select the module
  // according to the extension
  // ==============

  if( _extension.contains("mp3", false) ) {
    qDebug( "(K3bAudioModuleFactory) Creating K3bAudioModule for mp3..." );
    return new K3bMp3Module( track );
  }
  else if( _extension.contains("wav", false) ) {
    qDebug( "(K3bAudioModuleFactory) Creating K3bAudioModule for wav..." );
    return new K3bWavModule( track );
  }
  else {
    qDebug( "(K3bAudioModuleFactory) No K3bAudioModule availible." );
    return 0;
  }
}


bool K3bAudioModuleFactory::moduleAvailable( const KURL& url )
{
  QString _extension = QFileInfo( url.path() ).extension(false);

  // ==============
  // for now we do no deep filetype-checking but select the module
  // according to the extension
  // ==============

  if( _extension.contains("mp3", false) ) {
    return true;
  }
  else if( _extension.contains("wav", false) ) {
    unsigned long buffy1;
    long buffy2;
    if( K3bWavModule::waveLength( url.path().latin1(), 0, &buffy2, &buffy1 ) == 0 )
      return true;
    else
      return false;
  }
  else {
    return false;
  }
}
