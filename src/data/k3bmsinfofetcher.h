#ifndef K3B_MSINFO_FETCHER_H
#define K3B_MSINFO_FETCHER_H

#include "../k3bjob.h"


class K3bDevice;
class KProcess;

class K3bMsInfoFetcher : public K3bJob
{
  Q_OBJECT

 public:
  K3bMsInfoFetcher( QObject* parent = 0, const char* name = 0 );
  ~K3bMsInfoFetcher();

  const QString& msInfo() const { return m_msInfo; }
  int lastSessionStart() const { return m_lastSessionStart; }
  int nextSessionStart() const { return m_nextSessionStart; }

 public slots:
  void start();
  void cancel();

  void setDevice( K3bDevice* dev ) { m_device = dev; }

 private slots:
  void slotProcessExited();
  void slotCollectOutput( KProcess*, char* output, int len );

 private:
  QString m_msInfo;
  int m_lastSessionStart;
  int m_nextSessionStart;
  QString m_collectedOutput;

  KProcess* m_process;
  K3bDevice* m_device;

  bool m_canceled;
};

#endif
