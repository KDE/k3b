#ifndef K3BMP3MODULE_H
#define K3BMP3MODULE_H


#include "k3bexternalbinmodule.h"

class QTimer;
class KShellProcess;


class K3bMp3Module : public K3bExternalBinModule
{
  Q_OBJECT

 public:
  K3bMp3Module( K3bAudioTrack* track );
  ~K3bMp3Module();

  /** check if the url contains the correct filetype **/
//  bool valid() const;

  KURL writeToWav( const KURL& url );

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


  QTimer* m_rawDataCountTimer;
  KShellProcess* m_decodingProcess;
  long m_rawData;

  /**
   * Since we do not want to run several mpg123 
   * processes at a time we use a static variable
   * to check if some object is currenty working
   */
  static bool staticIsDataBeingEncoded;
};


#endif
