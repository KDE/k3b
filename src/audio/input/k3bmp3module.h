#ifndef K3BMP3MODULE_H
#define K3BMP3MODULE_H


#include "k3baudiomodule.h"

class KProcess;
class KShellProcess;
class QTimer;
class QFile;

class K3bMp3Module : public K3bAudioModule
{
  Q_OBJECT

 public:
  K3bMp3Module( K3bAudioTrack* track );
  ~K3bMp3Module();

  /** check if the url contains the correct filetype **/
//  bool valid() const;

  KURL writeToWav( const KURL& url );
  bool getStream();

  int readData( char*, int );

 public slots:
  void cancel();

 private:
  //  bool findFrameHeader( unsigned int );
  bool mp3HeaderCheck(unsigned int header);

  int mp3Padding(unsigned int header);
  int mp3SampleRate(unsigned int header);
  int mp3LayerNumber(unsigned int header);
  int mp3Bitrate(unsigned int header);
  int mp3VersionNumber(unsigned int header);
  bool mp3Protection(unsigned int header);

  double compute_tpf( unsigned int header );

 private slots:
  void slotStartCountRawData();

  void slotGatherInformation();
  void slotCountRawData(KProcess*, char*, int);
  void slotCountRawDataFinished();
  void slotParseStdErrOutput(KProcess*, char*, int);
  void slotDecodingFinished();
  void slotReceivedStdout(KProcess*, char*, int);

  void slotClearData();
  void slotWriteToWavFinished();

 private:
  QTimer* m_infoTimer;
  QTimer* m_clearDataTimer;

  KShellProcess* m_decodingProcess;
  char* m_currentData;
  int m_currentDataLength;

  /**
   * Since we do not want to run several mpg123 
   * processes at a time we use a static variable
   * to check if some object is currenty working
   */
  static bool staticIsDataBeingEncoded;

  long m_rawData;

  bool m_finished;

  // QFile* m_testFile;
};


#endif
