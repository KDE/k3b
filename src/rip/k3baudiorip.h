#ifndef K3B_AUDIO_RIP
#define K3B_AUDIO_RIP


#include <qobject.h>
#include <qcstring.h>


class K3bDevice;
class QTimer;
class K3bCdparanoiaLib;


class K3bAudioRip : public QObject
{
  Q_OBJECT

 public:
  K3bAudioRip( QObject* parent = 0 );
  ~K3bAudioRip();

  bool ripTrack( K3bDevice*, unsigned int track );

 public slots:
  void cancel();
  void setParanoiaMode( int mode ) { m_paranoiaMode = mode; }
  void setRetries( int r ) { m_paranoiaRetries = r; }

 signals:
  void output( const QByteArray& );
  void percent( int );
  void finished( bool );

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

  bool m_bInterrupt;
  bool m_bError;
};

#endif
