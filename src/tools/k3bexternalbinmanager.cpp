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

#include "k3bexternalbinmanager.h"

#include <kdebug.h>
#include <kprocess.h>
#include <kconfig.h>

#include <qstring.h>
#include <qregexp.h>
#include <qfile.h>
#include <qfileinfo.h>

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>



// static const char* vcdTools[] =  { "vcdxgen",
// 				   "vcdxbuild",
// 				   0 };

// static const char* transcodeTools[] =  { "transcode",
// 					 "tcprobe",
// 					 "tccat",
// 					 "tcscan",
// 					 "tcextract",
// 					 "tcdecode",
// 					 0 };

// static const char* binPrograms[] =  { "cdrecord",
// 				      "cdrdao",
// 				      "mkisofs",
// 				      0 };


QString K3bExternalBinManager::m_noPath = "";


// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALBINVERSION
//
// ///////////////////////////////////////////////////////////

K3bExternalBinVersion::K3bExternalBinVersion()
{
  setVersion( 0, 1 );
}

K3bExternalBinVersion::K3bExternalBinVersion( const K3bExternalBinVersion& v )
  : m_versionString( v.versionString() ),
    m_majorVersion( v.majorVersion() ),
    m_minorVersion( v.minorVersion() ),
    m_patchLevel( v.patchLevel() ),
    m_suffix( v.suffix() )
{
}

K3bExternalBinVersion::K3bExternalBinVersion( const QString& version )
{
  setVersion( version );
}

K3bExternalBinVersion::K3bExternalBinVersion( int majorVersion, 
					      int minorVersion, 
					      int patchlevel, 
					      const QString& suffix )
{
  setVersion( majorVersion, minorVersion, patchlevel, suffix );
}

void K3bExternalBinVersion::setVersion( const QString& v )
{
  m_versionString = v;

  // TODO: parse the string
}

void K3bExternalBinVersion::setVersion( int majorVersion, 
					int minorVersion, 
					int patchlevel, 
					const QString& suffix )
{
  m_majorVersion = majorVersion;
  m_minorVersion = minorVersion;
  m_patchLevel = patchlevel;
  m_suffix = suffix;
  m_versionString = createVersionString( majorVersion, minorVersion, patchlevel, suffix );
}

K3bExternalBinVersion& K3bExternalBinVersion::operator=( const QString& v )
{
  setVersion( v );
  return *this;
}

QString K3bExternalBinVersion::createVersionString( int majorVersion, 
						    int minorVersion, 
						    int patchlevel, 
						    const QString& suffix )
{
  QString s = QString::number(majorVersion);

  if( minorVersion > -1 ) {
    s.append( QString(".%1").arg(minorVersion) );
    if( patchlevel > -1 )
      s.append( QString(".%1").arg(patchlevel) );
  }

  if( !suffix.isNull() )
    s.append( suffix );

  return s;
}




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
  return version.isEmpty();
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

void K3bExternalProgram::addBin( K3bExternalBin* bin )
{
  if( !m_bins.contains( bin ) )
    m_bins.append( bin );
}

void K3bExternalProgram::setDefault( K3bExternalBin* bin )
{
  if( m_bins.contains( bin ) )
    m_bins.take( m_bins.find( bin ) );

  m_bins.insert( 0, bin );
}


void K3bExternalProgram::setDefault( const QString& path )
{
  for( QPtrListIterator<K3bExternalBin> it( m_bins ); it.current(); ++it ) {
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


K3bExternalProgram::OutputCollector::OutputCollector( KProcess* p )
  : m_process(0)
{
  setProcess( p );
}

void K3bExternalProgram::OutputCollector::setProcess( KProcess* p )
{
  if( m_process )
    disconnect( m_process );

  m_process = p;
  if( p ) {
    connect( p, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(slotGatherOutput(KProcess*, char*, int)) );
    connect( p, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(slotGatherOutput(KProcess*, char*, int)) );
  }

  m_gatheredOutput = "";
}

void K3bExternalProgram::OutputCollector::slotGatherOutput( KProcess*, char* data, int len )
{
  m_gatheredOutput.append( QString::fromLatin1( data, len ) );
}



// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALBINMANAGER
//
// ///////////////////////////////////////////////////////////


K3bExternalBinManager::K3bExternalBinManager()
  : QObject()
{
}


K3bExternalBinManager::~K3bExternalBinManager()
{
  clear();
}


// K3bExternalBin* K3bExternalBinManager::probeCdrecord( const QString& path )
// {
//   if( !QFile::exists( path ) )
//     return 0;

//   K3bExternalBin* bin = 0;

//   // probe version
//   KProcess vp;
//   vp << path << "-version";
//   connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     int pos = m_gatheredOutput.find( "Cdrecord" );
//     if( pos < 0 )
//       return 0;

//     pos = m_gatheredOutput.find( QRegExp("[0-9]"), pos );
//     if( pos < 0 )
//       return 0;

//     int endPos = m_gatheredOutput.find( ' ', pos+1 );
//     if( endPos < 0 )
//       return 0;

//     bin = new K3bExternalBin( program("cdrecord") );
//     bin->path = path;
//     bin->version = m_gatheredOutput.mid( pos, endPos-pos );
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
//     return 0;
//   }



//   // probe features
//   KProcess fp;
//   fp << path << "-help";
//   connect( &fp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   connect( &fp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     if( m_gatheredOutput.contains( "gracetime" ) )
//       bin->addFeature( "gracetime" );
//     if( m_gatheredOutput.contains( "-overburn" ) )
//       bin->addFeature( "overburn" );
//     if( m_gatheredOutput.contains( "-text" ) )
//       bin->addFeature( "cdtext" );
//     if( m_gatheredOutput.contains( "-clone" ) )  // cdrecord ProDVD
//       bin->addFeature( "clone" );

//     // check if we run cdrecord as root
//     if( !getuid() )
//       bin->addFeature( "suidroot" );
//     else {
//       struct stat s;
//       if( !::stat( QFile::encodeName(path), &s ) ) {
// 	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
// 	  bin->addFeature( "suidroot" );
//       }
//     }
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << bin->path << endl;
//     delete bin;
//     return 0;
//   }

//   return bin;
// }


// K3bExternalBin* K3bExternalBinManager::probeMkisofs( const QString& path )
// {
//   if( !QFile::exists( path ) )
//     return 0;

//   K3bExternalBin* bin = 0;

//   // probe version
//   KProcess vp;
//   vp << path << "-version";
//   connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     int pos = m_gatheredOutput.find( "mkisofs" );
//     if( pos < 0 )
//       return 0;

//     pos = m_gatheredOutput.find( QRegExp("[0-9]"), pos );
//     if( pos < 0 )
//       return 0;

//     int endPos = m_gatheredOutput.find( ' ', pos+1 );
//     if( endPos < 0 )
//       return 0;

//     bin = new K3bExternalBin( program("mkisofs") );
//     bin->path = path;
//     bin->version = m_gatheredOutput.mid( pos, endPos-pos );
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
//     return 0;
//   }



//   // probe features
//   KProcess fp;
//   fp << path << "-help";
//   connect( &fp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   connect( &fp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     if( m_gatheredOutput.contains( "-udf" ) )
//       bin->addFeature( "udf" );
//     if( m_gatheredOutput.contains( "-dvd-video" ) )
//       bin->addFeature( "dvd-video" );
//     if( m_gatheredOutput.contains( "-joliet-long" ) )
//       bin->addFeature( "joliet-long" );

//     // check if we run mkisofs as root
//     if( !getuid() )
//       bin->addFeature( "suidroot" );
//     else {
//       struct stat s;
//       if( !::stat( QFile::encodeName(path), &s ) ) {
// 	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
// 	  bin->addFeature( "suidroot" );
//       }
//     }
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << bin->path << endl;
//     delete bin;
//     return 0;
//   }

//   return bin;
// }


// K3bExternalBin* K3bExternalBinManager::probeCdrdao( const QString& path )
// {
//   if( !QFile::exists( path ) )
//     return 0;

//   K3bExternalBin* bin = 0;

//   // probe version
//   KProcess vp;
//   vp << path ;
//   connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     int pos = m_gatheredOutput.find( "Cdrdao version" );
//     if( pos < 0 )
//       return 0;

//     pos = m_gatheredOutput.find( QRegExp("[0-9]"), pos );
//     if( pos < 0 )
//       return 0;

//     int endPos = m_gatheredOutput.find( ' ', pos+1 );
//     if( endPos < 0 )
//       return 0;

//     bin = new K3bExternalBin( program("cdrdao") );
//     bin->path = path;
//     bin->version = m_gatheredOutput.mid( pos, endPos-pos );
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
//     return 0;
//   }



//   // probe features
//   KProcess fp;
//   fp << path << "write" << "-h";
//   connect( &fp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   connect( &fp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     if( m_gatheredOutput.contains( "--overburn" ) )
//       bin->addFeature( "overburn" );
//     if( m_gatheredOutput.contains( "--multi" ) )
//       bin->addFeature( "multisession" );

//     // check if we run cdrdao as root
//     if( !getuid() )
//       bin->addFeature( "suidroot" );
//     else {
//       struct stat s;
//       if( !::stat( QFile::encodeName(path), &s ) ) {
// 	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
// 	  bin->addFeature( "suidroot" );
//       }
//     }
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << bin->path << endl;
//     delete bin;
//     return 0;
//   }

//   return bin;
// }



// K3bExternalBin* K3bExternalBinManager::probeTranscode( const QString& path )
// {
//   if( !QFile::exists( path ) )
//     return 0;

//   K3bExternalBin* bin = 0;

//   // probe version
//   KProcess vp;
//   vp << path ;
//   connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     int pos = m_gatheredOutput.find( "transcode v" );
//     if( pos < 0 )
//       return 0;

//     pos += 11;

//     int endPos = m_gatheredOutput.find( QRegExp("[\\s\\)]"), pos+1 );
//     if( endPos < 0 )
//       return 0;

//     bin = new K3bExternalBin( program("transcode") );
//     bin->path = path;
//     bin->version = m_gatheredOutput.mid( pos, endPos-pos );
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
//     return 0;
//   }

//   return bin;
// }


// K3bExternalBin* K3bExternalBinManager::probeMovix( const QString& path )
// {
//   // first test if we have a version info (eMovix >= 0.8.0pre3)
//   if( !QFile::exists( path + "/movix-version") )
//     return 0;

//   K3bExternalBin* bin = 0;

//   // probe version
//   KProcess vp;
//   vp << path + "/movix-version" ;
//   connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     // movix-version just gives us the version number on stdout
//     if( !m_gatheredOutput.isEmpty() ) {
//       bin = new K3bExternalBin( program("eMovix") );
//       bin->version = m_gatheredOutput.stripWhiteSpace();
//     }
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << path << "/movix-version" << endl;
//     return 0;
//   }

//   // now search for the config and the files
//   if( !QFile::exists( path + "/movix-conf") ) {
//     delete bin;
//     return 0;
//   }

//   KProcess cp;
//   cp << path + "/movix-conf";
//   connect( &cp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( cp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     // now search the needed files in the given dir
//     if( m_gatheredOutput.isEmpty() ) {
//       kdDebug() << "(K3bExternalBinManager) no eMovix config info" << endl;
//       delete bin;
//       return 0;
//     }

//     // we need the following files:
//     // isolinux/initrd.gz
//     // isolinux/iso.sort
//     // isolinux/isolinux.bin
//     // isolinux/isolinux.cfg
//     // isolinux/movix.lss
//     // isolinux/mphelp.txt
//     // isolinux/mxhelp.txt
//     // isolinux/trblst.txt
//     // isolinux/credits.txt
//     // isolinux/kernel/vmlinuz

//     // TODO: search the files

//     // the eMovix bin does not contain the path to any executable but the path to the eMovix files
//     bin->path = m_gatheredOutput.stripWhiteSpace();
//     return bin;
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << path << "/movix-conf" << endl;
//     delete bin;
//     return 0;
//   }
// }


// K3bExternalBin* K3bExternalBinManager::probeVcd( const QString& path )
// {
//   if( !QFile::exists( path ) )
//     return 0;

//   K3bExternalBin* bin = 0;

//   // probe version
//   KProcess vp;
//   vp << path << "-V";
//   connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
//   m_gatheredOutput = "";
//   if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
//     int pos = m_gatheredOutput.find( "GNU VCDImager" );
//     if( pos < 0 )
//       return 0;

//     pos += 14;

//     int endPos = m_gatheredOutput.find( QRegExp("[\\n\\)]"), pos+1 );
//     if( endPos < 0 )
//       return 0;

//     bin = new K3bExternalBin( program("vcdxgen") );
//     bin->path = path;
//     bin->version = m_gatheredOutput.mid( pos, endPos-pos ).stripWhiteSpace();
//   }
//   else {
//     kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
//     return 0;
//   }

//   return bin;
// }

// void K3bExternalBinManager::gatherOutput( KProcess*, char* data, int len )
// {
//   m_gatheredOutput.append( QString::fromLatin1( data, len ) );
// }


bool K3bExternalBinManager::readConfig( KConfig* c )
{
  loadDefaultSearchPath();

  if( c->hasKey( "search path" ) )
    setSearchPath( c->readListEntry( "search path" ) );

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
  }

  return true;
}

bool K3bExternalBinManager::saveConfig( KConfig* c )
{
  c->writeEntry( "search path", m_searchPath );

  for ( QMap<QString, K3bExternalProgram*>::iterator it = m_programs.begin(); it != m_programs.end(); ++it ) {
    K3bExternalProgram* p = it.data();
    if( p->defaultBin() )
      c->writeEntry( p->name() + " default", p->defaultBin()->path );

    c->writeEntry( p->name() + " user parameters", p->userParameters() );
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

// void K3bExternalBinManager::createProgramContainer()
// {
//   for( int i = 0; binPrograms[i]; ++i ) {
//     if( m_programs.find( binPrograms[i] ) == m_programs.end() )
//       m_programs.insert( binPrograms[i], new K3bExternalProgram( binPrograms[i] ) );
//   }
//   for( int i = 0; transcodeTools[i]; ++i ) {
//     if( m_programs.find( transcodeTools[i] ) == m_programs.end() )
//       m_programs.insert( transcodeTools[i], new K3bExternalProgram( transcodeTools[i] ) );
//   }  
//   for( int i = 0; vcdTools[i]; ++i ) {
//     if( m_programs.find( vcdTools[i] ) == m_programs.end() )
//       m_programs.insert( vcdTools[i], new K3bExternalProgram( vcdTools[i] ) );
//   }

//   if( m_programs.find( "eMovix" ) == m_programs.end() )
//     m_programs.insert( "eMovix", new K3bExternalProgram( "eMovix" ) );
// }


void K3bExternalBinManager::search()
{
  if( m_searchPath.isEmpty() )
    loadDefaultSearchPath();

  //  createProgramContainer();

  for( QMap<QString, K3bExternalProgram*>::iterator it = m_programs.begin(); it != m_programs.end(); ++it ) {
    it.data()->clear();
  }

  // do not search one path twice
  QStringList paths;
  for( QStringList::const_iterator it = m_searchPath.begin(); it != m_searchPath.end(); ++it )
    if( !paths.contains( *it ) && !paths.contains( *it + "/" ) )
      paths.append(*it);

  // get the environment path variable
  char* env_path = ::getenv("PATH");
  if( env_path ) {
    QStringList env_pathList = QStringList::split(":", QString::fromLocal8Bit(env_path));
    for( QStringList::const_iterator it = env_pathList.begin(); it != env_pathList.end(); ++it )
      if( !paths.contains( *it ) && !paths.contains( *it + "/" ) )
	paths.append(*it);
  }


  for( QStringList::const_iterator it = paths.begin(); it != paths.end(); ++it ) {
    for( QMap<QString, K3bExternalProgram*>::iterator pit = m_programs.begin(); pit != m_programs.end(); ++pit )
      pit.data()->scan(*it);

    //    QString path = *it;
//     if( path[path.length()-1] == '/' )
//       path.truncate( path.length()-1 );

//     QFileInfo fi( path );
//     if( !fi.exists() )
//       continue;

//     K3bExternalBin* cdrecordBin = 0;
//     K3bExternalBin* mkisofsBin = 0;
//     K3bExternalBin* cdrdaoBin = 0;

//     if( fi.isDir() ) {
//        cdrecordBin = probeCdrecord( path + "/cdrecord" );
//        mkisofsBin = probeMkisofs( path + "/mkisofs" );
//        cdrdaoBin = probeCdrdao( path + "/cdrdao" );

//        for( int i = 0; transcodeTools[i]; ++i ) {
//            K3bExternalBin* bin = probeTranscode( path + "/" + QString::fromLatin1(transcodeTools[i]) );
//            if( bin )
//            m_programs[ transcodeTools[i] ]->addBin( bin );
//        }
//        for( int i = 0; vcdTools[i]; ++i ) {
//          K3bExternalBin* bin = probeVcd( path + "/" + QString::fromLatin1(vcdTools[i]) );
//          if( bin )
//           m_programs[ vcdTools[i] ]->addBin( bin );
//        }
//        K3bExternalBin* movixBin = probeMovix( path );
//        if( movixBin )
//          m_programs[ "eMovix" ]->addBin( movixBin );
//     }
//     else {
//        cdrecordBin = probeCdrecord( path );
//        mkisofsBin = probeMkisofs( path );
//        cdrdaoBin = probeCdrdao( path );

//        // TODO: is there any way to test the other tools?
//     }
//     if( cdrecordBin )
//       m_programs["cdrecord"]->addBin( cdrecordBin );
//     if( mkisofsBin )
//       m_programs["mkisofs"]->addBin( mkisofsBin );
//     if( cdrdaoBin )
//       m_programs["cdrdao"]->addBin( cdrdaoBin );
  }



  // TESTING
  // /////////////////////////
  const K3bExternalBin* bin = program("cdrecord")->defaultBin();

  if( !bin ) {
    kdDebug() << "(K3bExternalBinManager) Probing cdrecord failed" << endl;
  }
  else {
    kdDebug() << "(K3bExternalBinManager) Cdrecord " << bin->version << " features: "
	      << bin->features().join( ", " ) << endl;

    if( bin->version >= "1.11a02" )
      kdDebug() << "(K3bExternalBinManager) seems to be cdrecord version >= 1.11a02, using burnfree instead of burnproof" << endl;
    if( bin->version >= "1.11a31" )
      kdDebug() << "(K3bExternalBinManager) seems to be cdrecord version >= 1.11a31, support for Just Link via burnfree "
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



K3bExternalBinManager* K3bExternalBinManager::self()
{
  static K3bExternalBinManager* instance = 0;
  if( !instance )
    instance = new K3bExternalBinManager();
  return instance;
}


#include "k3bexternalbinmanager.moc"

