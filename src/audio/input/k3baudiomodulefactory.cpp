
#include "k3baudiomodulefactory.h"
#include "k3bmp3module.h"
#include "../k3baudiotrack.h"


#include <qstring.h>

#include <kurl.h>
#include <kfileitem.h>


K3bAudioModule* K3bAudioModuleFactory::createModule( K3bAudioTrack* track )
{
  KFileItem fileItem( -1, -1, KURL(track->absPath()) );

  if( fileItem.mimetype() == "audio/x-mp3" ) {
    qDebug( "(K3bAudioModuleFactory) Creating K3bAudioModule for mp3..." );
    return new K3bMp3Module( track );
  }
  else {
    qDebug( "(K3bAudioModuleFactory) No K3bAudioModule availible." );
    return 0;
  }
}


bool K3bAudioModuleFactory::moduleAvailable( const KURL& url )
{
  KFileItem fileItem( -1, -1, url );

  if( fileItem.mimetype() == "audio/x-mp3" ) {
    return true;
  }
  else {
    return false;
  }
}
