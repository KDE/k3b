#include <config.h>

#include "k3baudiomodulefactory.h"
#include "mp3/k3bmp3module.h"
#include "wave/k3bwavemodule.h"
#include "../k3baudiotrack.h"

#ifdef OGG_VORBIS
#include "ogg/k3boggvorbismodule.h"
#endif

#include <qstring.h>

#include <kurl.h>
#include <kfileitem.h>
#include <kdebug.h>


K3bAudioModuleFactory::K3bAudioModuleFactory()
  : QObject()
{
  // create an instance of all available modules 
  // (This should be plugins in the future)

  m_modules.append( new K3bWaveModule( this ) );

  m_modules.append( new K3bMp3Module( this ) );

#ifdef OGG_VORBIS
  m_modules.append( new K3bOggVorbisModule( this ) );
#endif
}


K3bAudioModuleFactory::~K3bAudioModuleFactory()
{
  // QObject should make sure the modules get deleted
}

K3bAudioModuleFactory* K3bAudioModuleFactory::self()
{
  static K3bAudioModuleFactory* factoryInstance = 0;
  if( factoryInstance == 0 ) {
    factoryInstance = new K3bAudioModuleFactory();
  }

  return factoryInstance;
}


K3bAudioModule* K3bAudioModuleFactory::createModule( K3bAudioTrack* track )
{
  KURL url;
  url.setPath( track->absPath() );

  for( QListIterator<K3bAudioModule> it( m_modules ); it.current(); ++it ) {
    if( it.current()->canDecode( url ) ) {
      it.current()->addTrackToAnalyse( track );
      return it.current();
    }
  }

  kdDebug() << "(K3bAudioModuleFactory) No K3bAudioModule availible." << endl;
  return 0;
}


bool K3bAudioModuleFactory::moduleAvailable( const KURL& url )
{
  for( QListIterator<K3bAudioModule> it( m_modules ); it.current(); ++it ) {
    if( it.current()->canDecode( url ) ) {
      return true;
    }
  }

  return false;
}


#include "k3baudiomodulefactory.moc"
