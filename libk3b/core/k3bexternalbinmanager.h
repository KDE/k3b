/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bversion.h"

class KConfig;
class KProcess;


class K3bExternalProgram;


/**
 * A K3bExternalBin represents an installed version of a program.
 * All K3bExternalBin objects are managed by K3bExternalPrograms.
 *
 * A bin may have certain features that are represented by a string.
 */
class K3bExternalBin
{
 public:
  K3bExternalBin( K3bExternalProgram* );
  virtual ~K3bExternalBin() {}

  K3bVersion version;
  QString path;
  QString copyright;

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
  const K3bExternalBin* mostRecentBin() const;

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

  /**
   * reimplement this if it does not make sense to have the user be able
   * to specify additional parameters
   */
  virtual bool supportsUserParameters() const { return true; }

 private:
  QString m_name;
  QStringList m_userParameters;
  QPtrList<K3bExternalBin> m_bins;
};


class K3bExternalBinManager : public QObject
{
  Q_OBJECT

 public:
  K3bExternalBinManager( QObject* parent = 0, const char* name = 0 );
  ~K3bExternalBinManager();

  void search();

  /**
   * read config and add changes to current map.
   * Takes care of setting the config group
   */
  bool readConfig( KConfig* );

  /**
   * Takes care of setting the config group
   */
  bool saveConfig( KConfig* );

  bool foundBin( const QString& name );
  const QString& binPath( const QString& name );
  const K3bExternalBin* binObject( const QString& name );
  const K3bExternalBin* mostRecentBinObject( const QString& name );

  K3bExternalProgram* program( const QString& ) const;
  const QMap<QString, K3bExternalProgram*>& programs() const { return m_programs; }

  /** always extends the default searchpath */
  void setSearchPath( const QStringList& );
  void addSearchPath( const QString& );
  void loadDefaultSearchPath();

  const QStringList& searchPath() const { return m_searchPath; }

  void addProgram( K3bExternalProgram* );
  void clear();

 private:
  QMap<QString, K3bExternalProgram*> m_programs;
  QStringList m_searchPath;

  static QString m_noPath;  // used for binPath() to return const string

  QString m_gatheredOutput;
};

#endif
