#ifndef K3B_EXTERNAL_BIN_MANAGER_H
#define K3B_EXTERNAL_BIN_MANAGER_H

#include <qmap.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>


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
  const QStringList& userParameters() const { return m_userParameters; }

  bool hasFeature( const QString& ) const;
  void addFeature( const QString& );
  void addUserParameter( const QString& );
  void clearUserParameters() { m_userParameters.clear(); }

 private:
  QString m_name;
  QStringList m_features;
  QStringList m_userParameters;
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

  QPtrList<K3bExternalBin> list() const;

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
