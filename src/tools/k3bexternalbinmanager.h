#ifndef K3B_EXTERNAL_BIN_MANAGER_H
#define K3B_EXTERNAL_BIN_MANAGER_H

#include <qmap.h>
#include <qobject.h>

static const char* binPrograms[] =  { "mkisofs", "cdrecord", "cdrdao",
                    "transcode", "tccat", "tcprobe", "tcscan", "tcextract" };
static const char* binVersions[] =  { "1.13", "1.9", "1.1.3",
                    "0.6.0pre3", "0.6.0pre3", "0.6.0pre3", "0.6.0pre3", "0.6.0pre3" };
static const char* searchPaths[] = { "/usr/bin/", "/usr/local/bin/",
				       "/usr/sbin/", "/usr/local/sbin/",
				       "/opt/schily/bin/" };

static const int NUM_BIN_PROGRAMS = 8;
static const int NUM_SEARCH_PATHS = 5;

class QString;
class KConfig;
class KProcess;


class K3bExternalBin
{
 public:
  K3bExternalBin( const QString& name );

  QString version;
  QString path;
  QString parameters;

  const QString& name() const;
  bool isEmpty() const;

 private:
  QString m_name;
};



class K3bExternalBinManager : public QObject
{
Q_OBJECT

 public:
  K3bExternalBinManager( QObject* parent = 0 );
  ~K3bExternalBinManager();

  void search();
  void checkVersions();

  /**
   * read config and add changes to current map
   */
  bool readConfig( KConfig* );
  bool saveConfig( KConfig* );

  bool foundBin( const QString& name );
  const QString& binPath( const QString& name );
  K3bExternalBin* binObject( const QString& name );

 private slots:
  void slotParseCdrdaoVersion( KProcess*, char* data, int len );
  void slotParseCdrtoolsVersion( KProcess*, char* data, int len );
  void slotParseOutputVersion( KProcess *p, char* data, int len );
  void slotParseTranscodeVersion( KProcess *p, char* data, int len );

 private:
  QMap<QString, K3bExternalBin*> m_binMap;
  KProcess* m_process;
  QString m_noPath;  // used for binPath() to return const string

  void searchVersion( int );
  void checkTranscodeVersion();
};

#endif
