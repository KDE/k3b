#ifndef K3BWAVMODULE_H
#define K3BWAVMODULE_H


#include "k3baudiomodule.h"

class QTimer;
class KShellProcess;
class KProcess;


class K3bWavModule : public K3bAudioModule
{
  Q_OBJECT

 public:
  K3bWavModule( K3bAudioTrack* track );
  ~K3bWavModule();

  /** check if the url contains the correct filetype **/
//  bool valid() const;

  KURL writeToWav( const KURL& url );
  bool getStream();

  static int waveLength(const char *filename, long offset,
			long *hdrlen, unsigned long *datalen);

  int readData( char*, int );

 public slots:
  void cancel();

 private slots:
  void slotParseStdErrOutput(KProcess*, char*, int);
  void slotOutputData(KProcess*, char*, int);
  //  void slotCountRawData(KProcess*, char*, int);
  void slotConvertingFinished();
  void slotClearData();

 private:
  QTimer* m_streamingTimer;
  QTimer* m_clearDataTimer;
  KShellProcess* m_convertingProcess;

  char* m_currentData;
  int m_currentDataLength;

  long m_rawData;

  bool m_finished;
};


#endif
