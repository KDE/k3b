#ifndef K3BMP3MODULE_H
#define K3BMP3MODULE_H


#include "k3bexternalbinmodule.h"

class KShellProcess;
class KURL;

class K3bMp3Module : public K3bExternalBinModule
{
  Q_OBJECT

 public:
  K3bMp3Module( K3bAudioTrack* track );
  ~K3bMp3Module();

  /** check if the url contains the correct filetype **/
//  bool valid() const;

  KURL writeToWav( const KURL& url );

  void init();

  void recalcLength();

 public slots:
  void cancel();

 protected:
  void addArguments();

 private slots:
  void slotStartCountRawData();
  void slotGatherInformation();
  void slotCountRawData(KProcess*, char*, int);
  void slotCountRawDataFinished();
  void slotParseStdErrOutput(KProcess*, char*, int);
  void slotWriteToWavFinished();

 private:
  bool mp3HeaderCheck(unsigned int header);
  int mp3Padding(unsigned int header);
  int mp3SampleRate(unsigned int header);
  int mp3LayerNumber(unsigned int header);
  int mp3Bitrate(unsigned int header);
  int mp3VersionNumber(unsigned int header);
  bool mp3Protection(unsigned int header);
  double compute_tpf( unsigned int header );

  KURL m_currentToWavUrl;
  KShellProcess* m_decodingProcess;
  long m_rawData;
};


#endif
