
#ifndef K3BDEVICEMANAGER_H
#define K3BDEVICEMANAGER_H

#include <qobject.h>
#include <qstring.h>
#include <qarray.h>
#include <qlist.h>

class KProcess;

class K3bDevice 
{
 public:
  K3bDevice() { bus = target = lun = maxReadSpeed = maxWriteSpeed = 0; burner = 0; burnproof = 0;}
  K3bDevice( K3bDevice * );
  K3bDevice( const K3bDevice& );
  K3bDevice( int _bus, int _target, int _lun,
	     const QString & _vendor,
	     const QString & _description,
	     const QString & _version,
	     bool _burner,
	     bool _burnproof,
	     int _maxReadSpeed,
	     const QString & _devicename, int _maxBurnSpeed = 0 )
    : bus( _bus ), target( _target ), lun( _lun ), vendor( _vendor ),
    description( _description ), version( _version ), burner( _burner ),
    burnproof( _burnproof ), maxReadSpeed( _maxReadSpeed ),
    devicename( _devicename ), maxWriteSpeed( _maxBurnSpeed ) 
    {}

  //      ~K3bDevice();

  QString device();

  int bus;
  int target;
  int lun;
  QString vendor;
  QString description;
  QString version;
  bool burner;
  bool burnproof;
  int maxReadSpeed;
  QString devicename;
  int maxWriteSpeed;
};


class K3bDeviceManager : public QObject 
{
  Q_OBJECT

 public:
  /**
   * Constructs a device-manager and scans the scsi-bus
   * for devices. Every instance of K3bDeviceManager on
   * a machine is equal, so having multible instances
   * does not make sense.
   **/
  K3bDeviceManager( QObject * parent );
  ~K3bDeviceManager();

  K3bDevice* deviceByBus( int, int, int );

  /**
   * Before getting the devices do a @ref scanbus().
   * @return List of all writer devices.
   */
  QList<K3bDevice>& burningDevices();

  /**
   * Note that all burning devices can also be used as
   * reading device and are not present in this list.
   * Before getting the devices do a @ref scanbus().
   * @return List of all reader devices without writer devices.
   **/
  QList<K3bDevice>& readingDevices();

  /** writes to stdout **/
  void printDevices();

  /**
   * Returns number of found devices and constructs
   * the lists m_burner and m_reader.
   **/
  int scanbus();

  /**
   * Reads the device information from the config file.
   */
  int readConfig();

  /**
   * Clears the writers and readers list of devices.
   */
  void clear();

 private:
  QList<K3bDevice> m_reader;
  QList<K3bDevice> m_writer;
  int m_foundDevices;

  static const int DEV_ARRAY_SIZE = 16;

};

#endif
