
#ifndef K3BDISKINFO_DETECTOR_H
#define K3BDISKINFO_DETECTOR_H

#include <qobject.h>

typedef Q_INT32 size32;

#include "k3bdiskinfo.h"


class K3bDevice;
class K3bTcWrapper;
class KProcess;


class K3bDiskInfoDetector : public QObject
{
  Q_OBJECT

 public:
  K3bDiskInfoDetector( QObject* parent = 0 );
  ~K3bDiskInfoDetector();

 public slots:
  void detect( K3bDevice* dev );
 
  /**
   * no diskInfoReady signal will be emitted 
   */
  void cancel();
  void finish(bool success);

 signals:
  void diskInfoReady( const K3bDiskInfo& info );

 private slots:
  void slotIsDvd( bool );
  void fetchDiskInfo();
  void fetchTocInfo();
  void testForDvd();
  void fetchIsoInfo();
  void calculateDiscId();

 private:
  K3bDevice* m_device;
  K3bDiskInfo m_info;
  K3bTcWrapper* m_tcWrapper;
  int m_cdfd;

  bool m_bCanceled;
};


#endif
