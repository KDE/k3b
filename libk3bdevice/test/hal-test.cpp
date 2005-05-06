#include "hal-test.h"

#include <qapplication.h>

int main( int argc, char** argv )
{
  QApplication app( argc, argv );
  Main m;
  app.setMainWidget( &m );
  m.show();
  return app.exec();
}
