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
  const QStringList& features() const { return m_features; }

  bool hasFeature( const QString& ) const;
  void addFeature( const QString& );
  void addUserParameter( const QString& );
  void clearUserParameters() { m_userParameters.clear(); }

 private:
  QString m_name;
  QStringList m_features;
  QStringList m_userParameters;
};


class K3bExternalProgram
{
 public:
  K3bExternalProgram( const QString& name );
  ~K3bExternalProgram();

  const K3bExternalBin* defaultBin() const { return m_bins.getFirst(); }

  void addUserParameter( const QString& );
  void setUserParameters( const QStringList& list ) { m_userParameters = list; }

  const QStringList& userParameters() const { return m_userParameters; }
  const QString& name() const { return m_name; }

  void addBin( K3bExternalBin* );
  void clear() { m_bins.clear(); }
  void setDefault( K3bExternalBin* );
  void setDefault( const QString& path );

  const QPtrList<K3bExternalBin>& bins() const { return m_bins; }

 private:
  QString m_name;
  QStringList m_userParameters;
  QPtrList<K3bExternalBin> m_bins;
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
  const K3bExternalBin* binObject( const QString& name );

  K3bExternalProgram* program( const QString& ) const;
  const QMap<QString, K3bExternalProgram*>& programs() const { return m_programs; }

  /** always extends the default searchpath */
  void setSearchPath( const QStringList& );
  void addSearchPath( const QString& );
  void loadDefaultSearchPath();

  const QStringList& searchPath() const { return m_searchPath; }

 private slots:
  void gatherOutput(KProcess*, char*, int);

 private:
  void createProgramContainer();

  K3bExternalBin* probeCdrecord( const QString& );
  K3bExternalBin* probeMkisofs( const QString& );
  K3bExternalBin* probeCdrdao( const QString& );
  K3bExternalBin* probeTranscode( const QString& );


  QMap<QString, K3bExternalProgram*> m_programs;
  QStringList m_searchPath;

  QString m_noPath;  // used for binPath() to return const string

  QString m_gatheredOutput;
};

#endif
