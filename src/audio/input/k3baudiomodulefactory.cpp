#include "../../../../config.h"

#include "k3baudiomodulefactory.h"
#include "k3bmp3module.h"
#include "../k3baudiotrack.h"

#ifdef OGG_VORBIS
#include "k3boggvorbismodule.h"
#endif

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

#ifdef OGG_VORBIS
  else if( K3bOggVorbisModule::canDecode( fileItem.url() ) ) {
    qDebug( "(K3bAudioModuleFactory) Creating K3bAudioModule for ogg vorbis..." );
    return new K3bOggVorbisModule( track );
  }
#endif

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

#ifdef OGG_VORBIS
  else if( K3bOggVorbisModule::canDecode( url ) )
    return true;
#endif

  else {
    return false;
  }
}
