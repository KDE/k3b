
#ifndef K3B_LAME_LIB_H
#define K3B_LAME_LIB_H


class QLibrary;

class K3bLameLib
{
 public:
  ~K3bLameLib();

  bool load();

  bool init();
  int getVersion();

  static K3bLameLib* self();

 private:
  K3bLameLib( QLibrary* );

  class Private;
  Private* d;
};


#endif
