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
  void getStream();

  static int waveLength(const char *filename, long offset,
			long *hdrlen, unsigned long *datalen);

 private slots:
  void slotParseStdErrOutput(KProcess*, char*, int);
  void slotOutputData(KProcess*, char*, int);
  //  void slotCountRawData(KProcess*, char*, int);
  void slotConvertingFinished();

  // test stuff
  void slotTestCountOutput(char*, int);
  void slotTestOutputFinished();

 private:
  QTimer* m_streamingTimer;
  KShellProcess* m_convertingProcess;

  long m_rawData;

  // test stuff
  long m_testRawData;
};


#endif
