#ifndef K3B_EXTERNAL_BIN_MANAGER_H
#define K3B_EXTERNAL_BIN_MANAGER_H

#include <qmap.h>
#include <qobject.h>


class QString;
class KConfig;
class KProcess;


class K3bExternalBin
{
 public:
  K3bExternalBin( const QString& name );

  QString version;
  QString path;

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

 private:
  QMap<QString, K3bExternalBin*> m_binMap;
  KProcess* m_process;
  QString m_noPath;  // used for binPath() to return const string
};

#endif
