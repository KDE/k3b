
#ifndef K3BDISKINFO_DETECTOR_H
#define K3BDISKINFO_DETECTOR_H

#include <qobject.h>

typedef Q_INT32 size32;

#include "k3bdiskinfo.h"
#include "../rip/k3btcwrapper.h"


class K3bDevice;


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
  void finish(bool success);

 signals:
  void diskInfoReady( const K3bDiskInfo& info );

 private slots:
  void fetchDiskInfo();
  void fetchTocInfo();
  void fetchIsoInfo();
  void calculateDiscId();
  void testForVideoDvd();
  void slotIsVideoDvd( bool dvd );

 private:
  K3bDevice* m_device;
  K3bDiskInfo m_info;
  K3bTcWrapper* m_tcWrapper;
  int m_cdfd;
};


#endif
