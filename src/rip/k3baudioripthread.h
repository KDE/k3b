#ifndef K3B_AUDIO_RIP_THREAD_H
#define K3B_AUDIO_RIP_THREAD_H

#include <qobject.h>
#include <qthread.h>
#include <qcstring.h>



class K3bDevice;
class QTimer;
class K3bCdparanoiaLib;


class K3bAudioRipThread : public QObject, public QThread
{
  Q_OBJECT

 public:
  K3bAudioRipThread( QObject* parent = 0 );
  ~K3bAudioRipThread();

 public slots:
  void start( QObject* eventReceiver = 0 );
  void cancel();
  void setParanoiaMode( int mode ) { m_paranoiaMode = mode; }
  void setMaxRetries( int r ) { m_paranoiaRetries = r; }
  void setNeverSkip( bool b ) { m_neverSkip = b; }
  void setDevice( K3bDevice* dev ) { m_device = dev; }
  void setTrackToRip( unsigned int track ) { m_track = track; }

 signals:
  void output( const QByteArray& );

 private:
  /** reimplemented from QThread. Does the work */
  void run();

  K3bCdparanoiaLib* m_paranoiaLib;
  K3bDevice* m_device;

  long m_currentSector;
  long m_lastSector;
  unsigned long m_sectorsRead;
  unsigned long m_sectorsAll;

  int m_paranoiaMode;
  int m_paranoiaRetries;
  bool m_neverSkip;

  unsigned int m_track;

  bool m_bInterrupt;
  bool m_bError;

  void createStatus(long, int);

  // status variables
  long m_lastReadSector;
  long m_overlap;
  long m_readSectors;

  QObject* m_eventReceiver;

  // this friend function will call createStatus(long,int)
  friend void paranoiaCallback(long, int);
};

#endif
