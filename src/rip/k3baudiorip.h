#ifndef K3B_AUDIO_RIP
#define K3B_AUDIO_RIP


#include <k3bjob.h>
#include <qcstring.h>



class K3bDevice;
class QTimer;
class K3bCdparanoiaLib;


class K3bAudioRip : public K3bJob
{
  Q_OBJECT

 public:
  K3bAudioRip( QObject* parent = 0 );
  ~K3bAudioRip();

 public slots:
  void start();
  void cancel();
  void setParanoiaMode( int mode ) { m_paranoiaMode = mode; }
  void setMaxRetries( int r ) { m_paranoiaRetries = r; }
  void setNeverSkip( bool b ) { m_neverSkip = b; }
  void setDevice( K3bDevice* dev ) { m_device = dev; }
  void setTrackToRip( unsigned int track ) { m_track = track; }

 signals:
  void output( const QByteArray& );

 protected slots:
  void slotParanoiaRead();
  void slotParanoiaFinished();

 private:
  K3bCdparanoiaLib* m_paranoiaLib;
  K3bDevice* m_device;
  QTimer* m_rippingTimer;

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

  // this friend function will call createStatus(long,int)
  friend void paranoiaCallback(long, int);
};

#endif
