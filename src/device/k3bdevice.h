
#ifndef K3BDEVICE_H
#define K3BDEVICE_H


#include <qstring.h>

class ScsiIf;



class K3bDevice 
{
 public:
  K3bDevice( const char* );
  K3bDevice( const QString & _vendor,
	     const QString & _description,
	     const QString & _version,
	     bool _burner,
	     bool _burnproof,
	     int _maxReadSpeed,
	     const QString & _devicename, int _maxBurnSpeed = 0 );

  ~K3bDevice();

  const QString& vendor() const { return m_vendor; }
  const QString& description() const { return m_description; }
  const QString& version() const { return m_version; }
  bool burner() const { return m_burner; }
  bool burnproof() const { return m_burnproof; }
  int maxReadSpeed() const { return m_maxReadSpeed; }
  const QString& devicename() const { return m_devicename; }
  int maxWriteSpeed() const { return m_maxWriteSpeed; }

  /** checks if unit is ready, returns:
   * <ul>
   *  <li>0: OK</li>
   *  <li>1: scsi command failed</li>
   *  <li>2: not ready</li>
   *  <li>3: not ready, no disk in drive</li>
   *  <li>4: not ready, tray out</li>
   * </ul>
   */
  int isReady() const;

  /** not quite sure if this is nessesary! */
  bool rezero() const;

  /**
   * Saves the cd's total number of available blocks in "length"
   * returns false if capacity could not be retrieved.
   */
  bool cdCapacity( long* );

  bool init();

 private:
  int getModePage( int pageCode, unsigned char *buf,
		   long bufLen, unsigned char *modePageHeader,
		   unsigned char *blockDesc, int showErrorMsg );

  QString m_vendor;
  QString m_description;
  QString m_version;
  bool m_burner;
  bool m_burnproof;
  int m_maxReadSpeed;
  QString m_devicename;
  int m_maxWriteSpeed;

  ScsiIf* m_scsiIf;
};


#endif
