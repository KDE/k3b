
#ifndef K3BDEVICEMANAGER_H
#define K3BDEVICEMANAGER_H

#include <qobject.h>
#include <qstring.h>
#include <qarray.h>
#include <qlist.h>

class KProcess;
class K3bDevice;


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

  K3bDevice* deviceByName( const QString& );

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

  K3bDevice* scanDevice( const char *dev, int showErrorMsg = 1 );
};

#endif
