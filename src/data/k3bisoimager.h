#ifndef K3B_ISO_IMAGER_H
#define K3B_ISO_IMAGER_H

#include "../k3bjob.h"

class K3bDataDoc;
class K3bDirItem;
class QTextStream;
class K3bProcess;
class K3bExternalBinManager;



class K3bIsoImager : public K3bJob
{
 Q_OBJECT

 public:
  K3bIsoImager( K3bExternalBinManager*, K3bDataDoc*, QObject* parent = 0, const char* name = 0 );
  ~K3bIsoImager();

 public slots:
  void start();
  void cancel();
  void calculateSize();

  void setMultiSessionInfo( const QString& );

  /**
   * after data has been emitted image creation will
   * be suspended until resume() is called
   */
  void resume();

 signals:
  void sizeCalculated( int exitCode, int size );

  /**
   * after data has been emitted image creation will
   * be suspended until resume() is called
   */
  void data( char* data, int len );

 private slots:
  void slotReceivedStdout( KProcess*, char*, int );
  void slotReceivedStderr( const QString& );
  void slotProcessExited( KProcess* );
  void slotCollectMkisofsPrintSizeStderr(KProcess*, char*, int);
  void slotCollectMkisofsPrintSizeStdout(KProcess*, char*, int);
  void slotMkisofsPrintSizeFinished();

 private:
  K3bExternalBinManager* m_externalBinManager;

  K3bDataDoc* m_doc;

  QString m_pathSpecFile;
  QString m_rrHideFile;
  QString m_jolietHideFile;

  bool m_noDeepDirectoryRelocation;

  bool m_importSession;
  QString m_multiSessionInfo;

  K3bProcess* m_process;

  // used for mkisofs -print-size parsing
  QString m_collectedMkisofsPrintSizeStdout;
  QString m_collectedMkisofsPrintSizeStderr;
  int m_mkisofsPrintSizeResult;

  bool writePathSpec( const QString& filename );
  bool writeRRHideFile( const QString& filename );
  bool writeJolietHideFile( const QString& filename );
  void writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream );
  QString escapeGraftPoint( const QString& str );
  bool addMkisofsParameters();
  bool prepareMkisofsFiles();

  void cleanup();
};


#endif
