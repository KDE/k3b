
#ifndef K3B_CDPARANOIA_LIB_H
#define K3B_CDPARANOIA_LIB_H

// from cdda_interface.h
#define CD_FRAMESIZE_RAW 2352


#include <qstring.h>

#include <sys/types.h>


class K3bCdparanoiaLib
{
 public:
  ~K3bCdparanoiaLib();

  bool paranoiaInit( const QString& devicename );
  void paranoiaFree();

  /** default: 3 */
  void setParanoiaMode( int );

  /** default: 20 */
  void setMaxRetries( int );

  int16_t* paranoiaRead(void(*callback)(long,int));
  long paranoiaSeek( long, int );

  long firstSector( int );
  long lastSector( int );

  /**
   * returns 0 if the cdparanoialib could not
   * be found on the system.
   * Otherwise you have to take care of
   * deleting.
   */
  static K3bCdparanoiaLib* create();

 private:
  K3bCdparanoiaLib();
  bool load();

  static void* s_libInterface;
  static void* s_libParanoia;
  static int s_counter;

  class Private;
  Private* d;
};


#endif
