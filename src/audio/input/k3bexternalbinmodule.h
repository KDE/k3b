#ifndef K3B_EXTERNAL_BIN_MODULE_H
#define K3B_EXTERNAL_BIN_MODULE_H


#include "k3baudiomodule.h"

class KProcess;
class KShellProcess;
class QTimer;
class QFile;


/**
 * Abstract module class that handles raw data output from
 * an external process. It ensures that the output
 * will be a multible of 2352 bytes and fit exactly the
 * length of the accociated track.
 */
class K3bExternalBinModule : public K3bAudioModule
{
  Q_OBJECT

 public:
  K3bExternalBinModule( K3bAudioTrack* track );
  virtual ~K3bExternalBinModule();

  bool getStream();
  int readData( char*, int );

  virtual KURL writeToWav( const KURL& url = KURL() ) = 0;

 public slots:
  virtual void cancel();

 protected:
 /**
  * add the arguments for your external process to 
  * m_process. It will be started by K3bExternalBinModule.
  */
  virtual void addArguments() = 0;

 protected slots:
   /**
    * This will be started by a timer and should
    * calculate length of the track and eventually 
    * other data.
    */
  virtual void slotGatherInformation() = 0;

 /**
  * parse your process' output to create progress or error 
  * information
  */
  virtual void slotParseStdErrOutput(KProcess*, char*, int) = 0;

 private slots:
  void slotProcessFinished();
  void slotReceivedStdout(KProcess*, char*, int);
  void slotClearData();

 protected:
  KShellProcess* m_process;

 private:
  QTimer* m_infoTimer;
  QTimer* m_clearDataTimer;

  char* m_currentData;
  int m_currentDataLength;
  long m_rawDataSoFar;
  bool m_finished;

  // QFile* m_testFile;
};


#endif
