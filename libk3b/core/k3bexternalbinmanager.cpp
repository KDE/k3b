/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bexternalbinmanager.h"

#include <kdebug.h>
#include <k3process.h>
#include <kconfig.h>
#include <kdeversion.h>

#include <qstring.h>
#include <qregexp.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <q3ptrlist.h>

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>



QString K3bExternalBinManager::m_noPath = "";


// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALBIN
//
// ///////////////////////////////////////////////////////////

K3bExternalBin::K3bExternalBin( K3bExternalProgram* p )
  : m_program(p)
{
}


bool K3bExternalBin::isEmpty() const
{
  return !version.isValid();
}


const QString& K3bExternalBin::name() const
{
  return m_program->name();
}


bool K3bExternalBin::hasFeature( const QString& f ) const
{
  return m_features.contains( f );
}


void K3bExternalBin::addFeature( const QString& f )
{
  m_features.append( f );
}


const QStringList& K3bExternalBin::userParameters() const
{
  return m_program->userParameters();
}



// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALPROGRAM
//
// ///////////////////////////////////////////////////////////


K3bExternalProgram::K3bExternalProgram( const QString& name )
  : m_name( name )
{
  m_bins.setAutoDelete( true );
}


K3bExternalProgram::~K3bExternalProgram()
{
}


const K3bExternalBin* K3bExternalProgram::mostRecentBin() const
{
  Q3PtrListIterator<K3bExternalBin> it( m_bins );
  K3bExternalBin* bin = *it;
  ++it;
  while( *it ) {
    if( it.current()->version > bin->version )
      bin = *it;
    ++it;
  }
  return bin;
}


void K3bExternalProgram::addBin( K3bExternalBin* bin )
{
  if( !m_bins.contains( bin ) ) {
    // insertion sort
    // the first bin in the list is always the one used 
    // so we default to using the newest one
    K3bExternalBin* oldBin = m_bins.first();
    while( oldBin && oldBin->version > bin->version )
      oldBin = m_bins.next();

    m_bins.insert( oldBin ? m_bins.at() : m_bins.count(), bin );
  }
}

void K3bExternalProgram::setDefault( const K3bExternalBin* bin )
{
  if( m_bins.contains( bin ) )
    m_bins.take( m_bins.find( bin ) );

  // the first bin in the list is always the one used 
  m_bins.insert( 0, bin );
}


void K3bExternalProgram::setDefault( const QString& path )
{
  for( Q3PtrListIterator<K3bExternalBin> it( m_bins ); it.current(); ++it ) {
    if( it.current()->path == path ) {
      setDefault( it.current() );
      return;
    }
  }
}


void K3bExternalProgram::addUserParameter( const QString& p )
{
  if( !m_userParameters.contains( p ) )
    m_userParameters.append(p);
}



// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALBINMANAGER
//
// ///////////////////////////////////////////////////////////


K3bExternalBinManager::K3bExternalBinManager( QObject* parent, const char* name )
  : QObject( parent, name )
{
}


K3bExternalBinManager::~K3bExternalBinManager()
{
  clear();
}


bool K3bExternalBinManager::readConfig( KConfig* c )
{
  loadDefaultSearchPath();

  c->setGroup( "External Programs" );

  if( c->hasKey( "search path" ) )
    setSearchPath( c->readPathEntry( "search path", QStringList() ) );

  search();

  for ( QMap<QString, K3bExternalProgram*>::iterator it = m_programs.begin(); it != m_programs.end(); ++it ) {
    K3bExternalProgram* p = it.data();
    if( c->hasKey( p->name() + " default" ) ) {
      p->setDefault( c->readEntry( p->name() + " default" ) );
    }
    if( c->hasKey( p->name() + " user parameters" ) ) {
      QStringList list = c->readListEntry( p->name() + " user parameters" );
      for( QStringList::iterator strIt = list.begin(); strIt != list.end(); ++strIt )
	p->addUserParameter( *strIt );
    }
    if( c->hasKey( p->name() + " last seen newest version" ) ) {
      K3bVersion lastMax( c->readEntry( p->name() + " last seen newest version" ) );
      // now search for a newer version and use it (because it was installed after the last
      // K3b run and most users would probably expect K3b to use a newly installed version)
      const K3bExternalBin* newestBin = p->mostRecentBin();
      if( newestBin && newestBin->version > lastMax )
	p->setDefault( newestBin );
    }
  }

  return true;
}

bool K3bExternalBinManager::saveConfig( KConfig* c )
{
  c->setGroup( "External Programs" );
  c->writePathEntry( "search path", m_searchPath );

  for ( QMap<QString, K3bExternalProgram*>::iterator it = m_programs.begin(); it != m_programs.end(); ++it ) {
    K3bExternalProgram* p = it.data();
    if( p->defaultBin() )
      c->writeEntry( p->name() + " default", p->defaultBin()->path );

    c->writeEntry( p->name() + " user parameters", p->userParameters() );

    const K3bExternalBin* newestBin = p->mostRecentBin();
    if( newestBin )
      c->writeEntry( p->name() + " last seen newest version", newestBin->version );
  }

  return true;
}


bool K3bExternalBinManager::foundBin( const QString& name )
{
  if( m_programs.find( name ) == m_programs.end() )
    return false;
  else
    return (m_programs[name]->defaultBin() != 0);
}


const QString& K3bExternalBinManager::binPath( const QString& name )
{
  if( m_programs.find( name ) == m_programs.end() )
    return m_noPath;

  if( m_programs[name]->defaultBin() != 0 )
    return m_programs[name]->defaultBin()->path;
  else
    return m_noPath;
}


const K3bExternalBin* K3bExternalBinManager::binObject( const QString& name )
{
  if( m_programs.find( name ) == m_programs.end() )
    return 0;

  return m_programs[name]->defaultBin();
}


void K3bExternalBinManager::addProgram( K3bExternalProgram* p )
{
  m_programs.insert( p->name(), p );
}


void K3bExternalBinManager::clear()
{
  for( QMap<QString, K3bExternalProgram*>::Iterator it = m_programs.begin(); it != m_programs.end(); ++it )
    delete it.data();
  m_programs.clear();
}


void K3bExternalBinManager::search()
{
  if( m_searchPath.isEmpty() )
    loadDefaultSearchPath();

  for( QMap<QString, K3bExternalProgram*>::iterator it = m_programs.begin(); it != m_programs.end(); ++it ) {
    it.data()->clear();
  }

  // do not search one path twice
  QStringList paths;
  for( QStringList::const_iterator it = m_searchPath.begin(); it != m_searchPath.end(); ++it ) {
    QString p = *it;
    if( p[p.length()-1] == '/' )
      p.truncate( p.length()-1 );
    if( !paths.contains( p ) && !paths.contains( p + "/" ) )
      paths.append(p);
  }

  // get the environment path variable
  char* env_path = ::getenv("PATH");
  if( env_path ) {
    QStringList env_pathList = QStringList::split(":", QString::fromLocal8Bit(env_path));
    for( QStringList::const_iterator it = env_pathList.begin(); it != env_pathList.end(); ++it ) {
      QString p = *it;
      if( p[p.length()-1] == '/' )
	p.truncate( p.length()-1 );
      if( !paths.contains( p ) && !paths.contains( p + "/" ) )
	paths.append(p);
    }
  }


  for( QStringList::const_iterator it = paths.begin(); it != paths.end(); ++it )
    for( QMap<QString, K3bExternalProgram*>::iterator pit = m_programs.begin(); pit != m_programs.end(); ++pit )
      pit.data()->scan(*it);

  // TESTING
  // /////////////////////////
  const K3bExternalBin* bin = program("cdrecord")->defaultBin();

  if( !bin ) {
    kDebug() << "(K3bExternalBinManager) Probing cdrecord failed";
  }
  else {
    kDebug() << "(K3bExternalBinManager) Cdrecord " << bin->version << " features: "
	      << bin->features().join( ", " ) << endl;

    if( bin->version >= K3bVersion("1.11a02") )
      kDebug() << "(K3bExternalBinManager) "
		<< bin->version.majorVersion() << " " << bin->version.minorVersion() << " " << bin->version.patchLevel()
		<< " " << bin->version.suffix()
		<< " seems to be cdrecord version >= 1.11a02, using burnfree instead of burnproof" << endl;
    if( bin->version >= K3bVersion("1.11a31") )
      kDebug() << "(K3bExternalBinManager) seems to be cdrecord version >= 1.11a31, support for Just Link via burnfree "
		<< "driveroption" << endl;
  }
}


K3bExternalProgram* K3bExternalBinManager::program( const QString& name ) const
{
  if( m_programs.find( name ) == m_programs.end() )
    return 0;
  else
    return m_programs[name];
}


void K3bExternalBinManager::loadDefaultSearchPath()
{
  static const char* defaultSearchPaths[] = { "/usr/bin/",
					      "/usr/local/bin/",
					      "/usr/sbin/",
					      "/usr/local/sbin/",
					      "/opt/schily/bin/",
					      "/sbin",
					      0 };

  m_searchPath.clear();
  for( int i = 0; defaultSearchPaths[i]; ++i ) {
    m_searchPath.append( defaultSearchPaths[i] );
  }
}


void K3bExternalBinManager::setSearchPath( const QStringList& list )
{
  loadDefaultSearchPath();

  for( QStringList::const_iterator it = list.begin(); it != list.end(); ++it ) {
    if( !m_searchPath.contains( *it ) )
      m_searchPath.append( *it );
  }
}


void K3bExternalBinManager::addSearchPath( const QString& path )
{
  if( !m_searchPath.contains( path ) )
    m_searchPath.append( path );
}



const K3bExternalBin* K3bExternalBinManager::mostRecentBinObject( const QString& name )
{
  if( K3bExternalProgram* p = program( name ) )
    return p->mostRecentBin();
  else
    return 0;
}

#include "k3bexternalbinmanager.moc"

