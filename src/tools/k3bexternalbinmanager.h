/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_EXTERNAL_BIN_MANAGER_H
#define K3B_EXTERNAL_BIN_MANAGER_H

#include <qmap.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>


class KConfig;
class KProcess;


class K3bExternalProgram;


class K3bExternalBinVersion 
{
 public:
  /**
   * construct an empty version object
   * with version 0.1
   */
  K3bExternalBinVersion();

  /**
   * copy constructor
   */
  K3bExternalBinVersion( const K3bExternalBinVersion& );

  /**
   * this constructor tries to parse the given version string
   */
  K3bExternalBinVersion( const QString& version );

  /**
   * sets the version and generates a version string from it
   */
  K3bExternalBinVersion( int majorVersion, int minorVersion, int pachlevel = -1, const QString& suffix = QString::null );

  /**
   * tries to parse the version string
   * used by the constructor
   */
  void setVersion( const QString& );

  /**
   * sets the version and generates a version string from it
   * used by the constructor
   *
   * If minorVersion or pachlevel are -1 they will not be used when generating the version string.
   */
  void setVersion( int majorVersion, int minorVersion = -1, int patchlevel = -1, const QString& suffix = QString::null );

  const QString& versionString() const { return m_versionString; }
  int majorVersion() const { return m_majorVersion; }
  int minorVersion() const { return m_minorVersion; }
  int patchLevel() const { return m_patchLevel; }
  const QString& suffix() const { return m_suffix; }

  /**
   * just to make it possible to use as a QString
   */
  operator const QString& () const { return m_versionString; }
  K3bExternalBinVersion& operator=( const QString& v );

  /**
   * If minorVersion or pachlevel are -1 they will not be used when generating the version string.
   * If minorVersion is -1 patchlevel will be ignored.
   */
  static QString createVersionString( int majorVersion, 
				      int minorVersion = -1, 
				      int patchlevel = -1, 
				      const QString& suffix = QString::null );

 private:
  QString m_versionString;
  int m_majorVersion;
  int m_minorVersion;
  int m_patchLevel;
  QString m_suffix;
};


class K3bExternalBin
{
 public:
  K3bExternalBin( K3bExternalProgram* );

  QString version;
  QString path;

  const QString& name() const;
  bool isEmpty() const;
  const QStringList& userParameters() const;
  const QStringList& features() const { return m_features; }

  bool hasFeature( const QString& ) const;
  void addFeature( const QString& );

  K3bExternalProgram* program() const { return m_program; }

 private:
  QStringList m_features;
  K3bExternalProgram* m_program;
};


/**
 * This is the main class that represents a program
 * It's scan method has to be reimplemented for every program
 * It manages a list of K3bExternalBin-objects that each represent
 * one installed version of the program.
 */
class K3bExternalProgram
{
 public:
  K3bExternalProgram( const QString& name );
  virtual ~K3bExternalProgram();

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

  /**
   * this scans for the program in the given path,
   * adds the found bin object to the list and returnes true.
   * if nothing could be found false is returned.
   */
  virtual bool scan( const QString& ) {return false;}//= 0;

  class OutputCollector;

 private:
  QString m_name;
  QStringList m_userParameters;
  QPtrList<K3bExternalBin> m_bins;
};


class K3bExternalProgram::OutputCollector : public QObject
{
  Q_OBJECT

 public:
  OutputCollector( KProcess* );
  void setProcess( KProcess* );

  const QString& output() const { return m_gatheredOutput; }

 private slots:
  void slotGatherOutput( KProcess*, char*, int );

 private:
  QString m_gatheredOutput;
  KProcess* m_process;
};


class K3bExternalBinManager : public QObject
{
  Q_OBJECT

 public:
  ~K3bExternalBinManager();

  static K3bExternalBinManager* self();

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

  void addProgram( K3bExternalProgram* );
  void clear();

/*  private slots: */
/*   void gatherOutput(KProcess*, char*, int); */

 private:
  K3bExternalBinManager();

/*   void createProgramContainer(); */

/*   K3bExternalBin* probeCdrecord( const QString& ); */
/*   K3bExternalBin* probeMkisofs( const QString& ); */
/*   K3bExternalBin* probeCdrdao( const QString& ); */
/*   K3bExternalBin* probeTranscode( const QString& ); */
/*   K3bExternalBin* probeMovix( const QString& ); */
/*   K3bExternalBin* probeVcd( const QString& ); */

  QMap<QString, K3bExternalProgram*> m_programs;
  QStringList m_searchPath;

  static QString m_noPath;  // used for binPath() to return const string

  QString m_gatheredOutput;
};

#endif
