
#ifndef K3BDISKINFO_DETECTOR_H
#define K3BDISKINFO_DETECTOR_H

#include <qobject.h>

#include "k3bdiskinfo.h"


class K3bDevice;
class K3bTcWrapper;

class K3bDiskInfoDetector : public QObject
{
  Q_OBJECT

 public:
  /** do NOT delete me. It's been taken care of! */
  static K3bDiskInfoDetector* detect( K3bDevice* );

 signals:
  void diskInfoReady( const K3bDiskInfo& info );

 protected:
  K3bDiskInfoDetector( K3bDevice* );
  
 private slots:
  void slotDetect();
  void delayedDestruct();
  void slotIsDvd( bool );

 private:
  K3bDevice* m_device;
  K3bDiskInfo m_info;
  K3bTcWrapper* m_tcWrapper;
};


#endif
