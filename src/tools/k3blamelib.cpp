#include "k3blamelib.h"

#include <qlibrary.h>
#include <kdebug.h>


QLibrary* K3bLameLib::s_lib = 0;
int K3bLameLib::s_counter = 0;


extern "C" {
  struct lame_global_flags;

  lame_global_flags* (*lame_lame_init)(void);
  int (*lame_lame_get_version)(const lame_global_flags*);
}



class K3bLameLib::Private
{
public:
  Private() {
    flags = 0;
  }
  ~Private() {
  }

  lame_global_flags* flags;
};


K3bLameLib::K3bLameLib()
{
  d = new Private();
  s_counter++;
}

K3bLameLib::~K3bLameLib()
{
  delete d;
  s_counter--;
  if( s_counter == 0 ) {
    delete s_lib;
    s_lib = 0;
  }
}


bool K3bLameLib::load()
{
  lame_lame_init = (lame_global_flags* (*) (void))s_lib->resolve( "lame_init" );
  lame_lame_get_version = (int (*) (const lame_global_flags*))s_lib->resolve( "lame_get_version" );
  
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


K3bLameLib* K3bLameLib::create()
{
  // check if lamelib is avalilable
  if( s_lib == 0 ) {
    s_lib = new QLibrary( "mp3lame" );
    if( !s_lib->load() ) {
      kdDebug() << "(K3bLameLib) Error while loading libLame. " << endl;
      delete s_lib;
      s_lib = 0;
      return 0;
    }
  }

  K3bLameLib* lib = new K3bLameLib();
  if( !lib->load() ) {
    kdDebug() << "(K3bLameLib) Error: could not resolve all symbols!" << endl;
    delete lib;
    return 0;
  }
  return lib;
}
