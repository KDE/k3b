#ifndef K3B_AUDIO_MODULE_FACTORY
#define K3B_AUDIO_MODULE_FACTORY


#include <qobject.h>
#include <qptrlist.h>

class K3bAudioTrack;
class K3bAudioModule;
class KURL;

class K3bAudioModuleFactory : public QObject
{
  Q_OBJECT

 public:
  ~K3bAudioModuleFactory();

  /** returns NULL if no Module for that type of file is available.
      for now these are static but in the future there could be some plugin
      mode with dynamicly loaded audiomodules */
  K3bAudioModule* createModule( K3bAudioTrack* );
  bool moduleAvailable( const KURL& url );

  static K3bAudioModuleFactory* self();

 private:
  K3bAudioModuleFactory();

  QPtrList<K3bAudioModule> m_modules;
};

#endif
