#ifndef K3B_AUDIO_RIP
#define K3B_AUDIO_RIP


#include <qobject.h>
#include <qcstring.h>

typedef Q_INT32 size32;
typedef Q_INT16 size16;

extern "C" {
#include <cdda_interface.h>
#include <cdda_paranoia.h>
}


class K3bDevice;
class QTimer;



class K3bAudioRip : public QObject
{
  Q_OBJECT

 public:
  K3bAudioRip( QObject* parent = 0 );
  ~K3bAudioRip();

  bool ripTrack( K3bDevice*, unsigned int track );

 public slots:
  void cancel();

 signals:
  void output( const QByteArray& );
  void percent( int );
  void finished( bool );

 protected slots:
  void slotParanoiaRead();
  void slotParanoiaFinished();

 private:
  cdrom_paranoia* m_paranoia;
  K3bDevice* m_device;
  QTimer* m_rippingTimer;

  long m_currentSector;
  long m_lastSector;
  unsigned long m_sectorsRead;
  unsigned long m_sectorsAll;

  bool m_bInterrupt;
  bool m_bError;
};

#endif
