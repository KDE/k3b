#ifndef K3B_AUDIO_MODULE_FACTORY
#define K3B_AUDIO_MODULE_FACTORY


class K3bAudioTrack;
class K3bAudioModule;
class KURL;

class K3bAudioModuleFactory
{
 public:
/*   K3bAudioModuleFactory(); */
/*   ~K3bAudioModuleFactory(); */

  /** returns NULL if no Module for that type of file is available.
      for now these are static but in the future there could be some plugin
      mode with dynamicly loaded audiomodules */
  static K3bAudioModule* createModule( K3bAudioTrack* );
  static bool moduleAvailable( const KURL& url );
};

#endif
