
#ifndef K3BDEVICE_H
#define K3BDEVICE_H


#include <qstring.h>

struct cdrom_drive;
class K3bToc;


class K3bDevice 
{
 public:
  /**
   * The available cdrdao drivers
   */
  static const char* cdrdao_drivers[13];
  enum interface { SCSI, IDE };

  /**
   * create a K3bDevice from a cdrom_drive struct
   * (cdparanoia-lib)
   */
  K3bDevice( cdrom_drive* );
  virtual ~K3bDevice();

  /**
   * opens a cdrom_drive which then can be used
   * for cdparanoia-methods like if it was opened
   * with cdda_open().
   * It must be closed with K3bDevice::close() after
   * using it!
   * This is somewhat buggy since it does not work anymore
   * if someone calls cdda_close after opening the drive
   * with this method! So please do NOT use cdda_open and
   * cdda_close!!!
   */
  virtual cdrom_drive* open();
  virtual bool close();

  virtual int interfaceType() const = 0;

  virtual const QString& vendor() const { return m_vendor; }
  virtual const QString& description() const { return m_description; }
  virtual const QString& version() const { return m_version; }
  virtual bool           burner() const { return m_burner; }
  virtual bool           writesCdrw() const { return m_bWritesCdrw; }
  virtual bool           burnproof() const { return m_burnproof; }
  virtual int            maxReadSpeed() const { return m_maxReadSpeed; }
  virtual int            currentWriteSpeed() const { return m_currentWriteSpeed; }

  virtual int            bufferSize() const { return m_bufferSize; }

  /**
   * returns genericDevice if not null
   * otherwise ioctlDevice is returned
   */
  const QString& devicename() const;

  /**
   * needed for the external programs
   */
  virtual const QString& genericDevice() const;

  /**
   * needed for mounting the drive
   */
  virtual const QString& ioctlDevice() const;

  const QString& mountDevice() const { return m_mountDevice; }

  /** makes only sense to use with sg devices */
  virtual QString busTargetLun() const;

  virtual int            maxWriteSpeed() const { return m_maxWriteSpeed; }
  virtual const QString& cdrdaoDriver() const { return m_cdrdaoDriver; }

  const QString& mountPoint() const { return m_mountPoint; }

  /**
   * returns: 0 auto (no cdrdao-driver selected)
   *          1 yes
   *          2 no
   */
  virtual int cdTextCapable() const;


  /** internally K3b value. */
  virtual void setCurrentWriteSpeed( int s ) { m_currentWriteSpeed = s; }


  virtual void setIsWriter( bool b ) { m_burner = b; }

  /**
   * Use this if the speed was not detected correctly.
   */
  virtual void setMaxReadSpeed( int s ) { m_maxReadSpeed = s; }

  /**
   * Use this if the speed was not detected correctly.
   */
  virtual void setMaxWriteSpeed( int s ) { m_maxWriteSpeed = s; }

  /**
   * Use this if cdrdao is not able to autodetect the nessessary driver.
   */
  virtual void setCdrdaoDriver( const QString& d ) { m_cdrdaoDriver = d; }

  /**
   * Only used if the cdrdao-driver is NOT set to "auto".
   * In that case it must be manually set because there
   * is no way to autosense the cd-text capability.
   */
  virtual void setCdTextCapability( bool );

  virtual void setBurnproof( bool );
  virtual void setWritesCdrw( bool b ) { m_bWritesCdrw = b; }
  virtual void setBufferSize( int b ) { m_bufferSize = b; }

  void setMountPoint( const QString& );
  void setMountDevice( const QString& d ) { m_mountDevice = d; }

  /** checks if unit is ready, returns:
   * <ul>
   *  <li>0: OK</li>
   *  <li>1: scsi command failed</li>
   *  <li>2: not ready</li>
   *  <li>3: not ready, no disk in drive</li>
   *  <li>4: not ready, tray out</li>
   * </ul>
   */
  virtual int isReady() const;

  /** not quite sure if this is nessesary! */
  //  virtual bool rezero() const;

  /**
   * Saves the cd's total number of available blocks in "length"
   * returns false if capacity could not be retrieved.
   * DOES NOT WORK SO FAR!
   */
/*   bool cdCapacity( long* ); */


  /**
   *  checks if disk is empty, returns:
   *  <li> 0: disk is empty</li>
   *  <li> 1: disk is not empty, but appendable</li>
   *  <li> 2: disk is complete</li>
   *  <li>-1: not ready, no disk in drive</li>
   * </ul>
   */
  virtual int isEmpty();

  virtual bool rewritable() { return false; }

  /**
   * block or unblock the drive's tray
   * returns true on success and false on scsi-error
   */
  virtual bool block( bool ) const;

  void eject() const;

 protected:
  QString m_vendor;
  QString m_description;
  QString m_version;
  bool m_burner;
  bool m_bWritesCdrw;
  bool m_burnproof;
  QString m_cdrdaoDriver;
  int m_cdTextCapable;
  int m_maxReadSpeed;
  int m_maxWriteSpeed;
  int m_currentWriteSpeed;

  cdrom_drive* m_cdromStruct;

  // only needed for scsi devices
  int m_bus;
  int m_target;
  int m_lun;

  int m_bufferSize;

 private:
  QString m_genericDevice;
  QString m_ioctlDevice;
  QString m_mountDevice;
  QString m_mountPoint;

  friend class K3bDeviceManager;
};


#endif
