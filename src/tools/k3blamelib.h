
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

  /**
   * returns 0 if the lamelib could not
   * be found on the system.
   * Otherwise you have to take care of
   * deleting.
   */
  static K3bLameLib* create();

 private:
  K3bLameLib( QLibrary* );

  class Private;
  Private* d;
};


#endif
