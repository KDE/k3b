#ifndef K3BWAVMODULE_H
#define K3BWAVMODULE_H


#include "k3bexternalbinmodule.h"

class QTimer;


class K3bWavModule : public K3bExternalBinModule
{
  Q_OBJECT

 public:
  K3bWavModule( K3bAudioTrack* track );
  ~K3bWavModule();

  /** check if the url contains the correct filetype **/
//  bool valid() const;

  KURL writeToWav( const KURL& url );

  static int waveLength(const char *filename, long offset,
			long *hdrlen, unsigned long *datalen);

 protected:
  void addArguments();

 protected slots:
  void slotParseStdErrOutput(KProcess*, char*, int);
  void slotWavFinished();

 private:
  QTimer* m_streamingTimer;
};


#endif
