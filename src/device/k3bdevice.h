
#ifndef K3BDEVICE_H
#define K3BDEVICE_H


#include <qstring.h>

class ScsiIf;


/* struct K3bDiskInfo { */
/*   long capacity;          // recordable capacity of medium */
/*   Msf  manufacturerId;    // disk identification */
/*   int  recSpeedLow;       // lowest recording speed */
/*   int  recSpeedHigh;      // highest recording speed */
  
/*   int sessionCnt;         // number of closed sessions */
/*   int lastTrackNr;        // number of last track on disk */
  
/*   long lastSessionLba;    // start lba of first track of last closed session */
/*   long thisSessionLba;    // start lba of this session */
  
/*   int diskTocType;        // type of CD TOC, only valid if CD-R is not empty */
  
/*   unsigned int empty  : 1; // 1 for empty disk, else 0 */
/*   unsigned int append : 1; // 1 if CD-R is appendable, else 0 */
/*   unsigned int cdrw   : 1; // 1 if disk is a CD-RW */
  
/*   struct { */
/*     unsigned int empty : 1; */
/*     unsigned int append : 1; */
/*     unsigned int cdrw : 1; */
/*     unsigned int capacity : 1; */
/*     unsigned int manufacturerId : 1; */
/*       unsigned int recSpeed : 1; */
/*   } valid; */
/* }; */


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
