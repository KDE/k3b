#include "k3blamelib.h"

#include <qlibrary.h>
#include <kdebug.h>


extern "C" {
  struct lame_global_flags;

  lame_global_flags* (*lame_lame_init)(void);
  int (*lame_lame_get_version)(const lame_global_flags*);
}



class K3bLameLib::Private
{
public:
  Private() {
    lib = 0;
    flags = 0;
  }
  ~Private() {
    delete lib;
  }

  lame_global_flags* flags;
  QLibrary* lib;
};


K3bLameLib::K3bLameLib( QLibrary* lib )
{
  d = new Private();
  d->lib = lib;
}

K3bLameLib::~K3bLameLib()
{
  delete d;
}


bool K3bLameLib::load()
{
  lame_lame_init = (lame_global_flags* (*) (void))d->lib->resolve( "lame_init" );
  lame_lame_get_version = (int (*) (const lame_global_flags*))d->lib->resolve( "lame_get_version" );

  // check if all symbols could be resoled
  if( lame_lame_init == 0 ) {
    kdDebug() << "(K3bLameLib) Error: could not resolve 'lame_init'" << endl;
    return false;
  }
  else if( lame_lame_get_version == 0 ) {
    kdDebug() << "(K3bLameLib) Error: could not resolve 'lame_get_version'" << endl;
    return false;
  }

  return true;
}

bool K3bLameLib::init()
{
  d->flags = lame_lame_init();
  return ( d->flags != 0 );
}


int K3bLameLib::getVersion()
{
  return lame_lame_get_version( d->flags );
}


K3bLameLib* K3bLameLib::self()
{
  static K3bLameLib* lib = 0;

  if( !lib ) {
    // check if lamelib is avalilable
    QLibrary* lameLib = new QLibrary( "mp3lame" );
    if( !lameLib->load() ) {
      kdDebug() << "(K3bLameLib) Error while loading libLame. " << endl;
      delete lameLib;
      return 0;
    }
    else {
      lib = new K3bLameLib( lameLib );
      if( !lib->load() ) {
	kdDebug() << "(K3bLameLib) Error: could not resolve all symbols!" << endl;
	delete lib;
	return 0;
      }
	
    }
  }
  
  return lib;
}
