#ifndef K3B_BLANKING_JOB_H
#define K3B_BLANKING_JOB_H

#include "k3bjob.h"

class KProcess;
class QString;
class K3bDevice;


class K3bBlankingJob : public K3bJob
{
Q_OBJECT

 public:
  K3bBlankingJob();
  ~K3bBlankingJob();

  enum blank_mode { Fast, Complete, Track, Unclose, Session };

 public slots:
  void start();
  void cancel();
  void setForce( bool f ) { m_force = f; }
  void setDevice( K3bDevice* d );
  void setSpeed( int s ) { m_speed = s; }
  void setMode( int m ) { m_mode = m; }

 private slots:
  void slotParseCdrecordOutput( KProcess*, char*, int );
  void slotCdrecordFinished();

 private:
  KProcess* m_process;
  bool m_force;
  K3bDevice* m_device;
  int m_speed;
  int m_mode;
};

#endif
