#ifndef K3B_EXTERNAL_BIN_MANAGER_H
#define K3B_EXTERNAL_BIN_MANAGER_H

#include <qmap.h>
#include <qobject.h>
#include <qstring.h>

// binary program name without path
// to add a new one add the name at the end of the array and implement your own
// version of checkVersions and add a case in slotParseOutputVersion.
//static const char* binPrograms[] =  { "mkisofs", "cdrecord", "cdrdao", "mpg123", "sox", "transcode", "tccat", "tcprobe", "tcscan", "tcextract", "tcdecode" };
// command argument to show the version number of the programs to check
//static const char* binVersionFlag[] =  { "--version", "--version", "--version", "--version", "-h", "-version", "--version", "-version", "-version", "-version", "-v" };
// minimum version of the programs
//static const char* binVersions[] =  { "1.13", "1.9", "1.1.3", "unknown", "unknown", "0.6.0pre3", "0.6.0pre3", "0.6.0pre3", "0.6.0pre3", "0.6.0pre3", "0.6.0pre3" };

static const char* binPrograms[] =  { "mkisofs", "cdrecord", "cdrdao",
                    "transcode", "tccat", "tcprobe", "tcscan", "tcextract", "tcdecode" };
static const char* binVersions[] =  { "1.13", "1.9", "1.1.3",
                    "0.6.0pre3", "0.6.0pre3", "0.6.0pre3", "0.6.0pre3", "0.6.0pre3", "0.6.0pre3" };
static const char* binVersionFlag[] =  { "--version", "--version", "--version", "--version", "-h", "-version", "--version", "-version", "-version", "-version", "-v" };

static const char* searchPaths[] = { "/usr/bin/", "/usr/local/bin/",
				       "/usr/sbin/", "/usr/local/sbin/",
				       "/opt/schily/bin/" };

//static const int NUM_BIN_PROGRAMS = 11; // 11
static const int NUM_BIN_PROGRAMS = 9;
static const int NUM_SEARCH_PATHS = 5;

#define TRANSCODE_START 3
#define TRANSCODE_END 9

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
  //void slotProcessExited( KProcess *p );

 private:
  QMap<QString, K3bExternalBin*> m_binMap;
  KProcess* m_process;
  QString m_noPath;  // used for binPath() to return const string
  //unsigned int m_programArrayIndex;
  //QString m_bin; // binary path/program to test

  void searchVersion( int );
  void checkTranscodeVersion();
};

#endif
