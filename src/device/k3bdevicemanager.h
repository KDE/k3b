
#ifndef K3BDEVICEMANAGER_H
#define K3BDEVICEMANAGER_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qarray.h>
#include <qlist.h>

#include <kdebug.h>

class KProcess;
class K3bDevice;
class KConfig;
class K3bExternalBinManager;
class cdrom_drive;


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
  K3bDeviceManager( K3bExternalBinManager*, QObject * parent );
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

  QList<K3bDevice>& allDevices();


  /** writes to stderr **/
  void printDevices();

  /**
   * Returns number of found devices and constructs
   * the lists m_burner and m_reader.
   **/
  int scanbus();

  /**
   * Reads the device information from the config file.
   */
  bool readConfig( KConfig* );

  bool saveConfig( KConfig* );

  /**
   * Clears the writers and readers list of devices.
   */
  void clear();

  /**
   * add a new device like "/dev/mebecdrom" to be sensed
   * by the deviceManager.
   */
  K3bDevice* addDevice( const QString& );

 private slots:
  void slotCollectStdout( KProcess*, char* data, int len );

 private:
 K3bExternalBinManager* m_externalBinManager;

  QList<K3bDevice> m_reader;
  QList<K3bDevice> m_writer;
  QList<K3bDevice> m_allDevices;;
  int m_foundDevices;

  QString m_processOutput;

  void scanFstab();
  K3bDevice* initializeScsiDevice( cdrom_drive* );
  K3bDevice* initializeIdeDevice( cdrom_drive* );

  static const int DEV_ARRAY_SIZE = 19;
  static const char* deviceNames[DEV_ARRAY_SIZE];
};

#endif
