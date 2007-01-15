/* 
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bdefaultexternalprograms.h"
#include "k3bexternalbinmanager.h"
#include <k3bglobals.h>

#include <qfile.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qobject.h>
#include <qregexp.h>
#include <qtextstream.h>

#include <k3bprocess.h>
#include <kdebug.h>

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>



void K3b::addDefaultPrograms( K3bExternalBinManager* m )
{
  m->addProgram( new K3bCdrecordProgram(false) );
  m->addProgram( new K3bMkisofsProgram() );
  m->addProgram( new K3bReadcdProgram() );
  m->addProgram( new K3bCdrdaoProgram() );
  m->addProgram( new K3bGrowisofsProgram() );
  m->addProgram( new K3bDvdformatProgram() );
  //  m->addProgram( new K3bDvdBooktypeProgram() );
}


void K3b::addTranscodePrograms( K3bExternalBinManager* m )
{
  static const char* transcodeTools[] =  { "transcode",
					   0, // K3b 1.0 only uses the transcode binary
					   "tcprobe",
					   "tccat",
					   "tcscan",
					   "tcextract",
					   "tcdecode",
					   0 };

  for( int i = 0; transcodeTools[i]; ++i )
    m->addProgram( new K3bTranscodeProgram( transcodeTools[i] ) );
}


void K3b::addVcdimagerPrograms( K3bExternalBinManager* m )
{
  // don't know if we need more vcdTools in the future (vcdxrip)
  static const char* vcdTools[] =  { "vcdxbuild",
                                     "vcdxminfo",
                                     "vcdxrip",
                                     0 };
  
  for( int i = 0; vcdTools[i]; ++i )
    m->addProgram( new K3bVcdbuilderProgram( vcdTools[i] ) );
}


K3bCdrecordProgram::K3bCdrecordProgram( bool dvdPro )
  : K3bExternalProgram( dvdPro ? "cdrecord-prodvd" : "cdrecord" ),
    m_dvdPro(dvdPro)
{
}


//
// This is a hack for Debian based systems which use
// a wrapper cdrecord script to call cdrecord.mmap or cdrecord.shm
// depending on the kernel version.
// For 2.0.x and 2.2.x kernels the shm version is used. In all
// other cases it's the mmap version.
//
// But since it may be that someone manually installed cdrecord 
// replacing the wrapper we check if cdrecord is a script.
//
static QString& debianWeirdnessHack( QString& path )
{
  if( QFile::exists( path + ".mmap" ) ) {
    kdDebug() << "(K3bCdrecordProgram) checking for Debian cdrecord wrapper script." << endl;
    if( QFileInfo( path ).size() < 1024 ) {
      kdDebug() << "(K3bCdrecordProgram) Debian Wrapper script size fits. Checking file." << endl;
      QFile f( path );
      f.open( IO_ReadOnly );
      QString s = QTextStream( &f ).read();
      if( s.contains( "cdrecord.mmap" ) && s.contains( "cdrecord.shm" ) ) {
	kdDebug() << "(K3bCdrecordProgram) Found Debian Wrapper script." << endl;
	QString ext;
	if( K3b::kernelVersion().versionString().left(3) > "2.2" )
	  ext = ".mmap";
	else
	  ext = ".shm";
	
	kdDebug() << "(K3bCdrecordProgram) Using cdrecord" << ext << endl;
	
	path += ext;
      }
    }
  }

  return path;
}


bool K3bCdrecordProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  bool wodim = false;
  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");

    if( QFile::exists( path + "cdrecord" ) ) {
      path += "cdrecord";
    }
    else if( QFile::exists( path + "wodim" ) ) {
      wodim = true;
      path += "wodim";
    }
  }

  debianWeirdnessHack( path );

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  K3bProcessOutputCollector out( &vp );

  vp << path << "-version";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = -1;
    if( wodim ) {
      pos = out.output().find( "Wodim" );
    }
    else if( m_dvdPro ) {
      pos = out.output().find( "Cdrecord-ProDVD" );
    }
    else {
      pos = out.output().find( "Cdrecord" );
    }

    if( pos < 0 )
      return false;

    pos = out.output().find( QRegExp("[0-9]"), pos );
    if( pos < 0 )
      return false;

    int endPos = out.output().find( ' ', pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );

    if( wodim )
      bin->addFeature( "wodim" );

    pos = out.output().find( "Copyright") + 14;
    endPos = out.output().find( "\n", pos );

    // cdrecord does not use local encoding for the copyright statement but plain latin1
    bin->copyright = QString::fromLatin1( out.output().mid( pos, endPos-pos ).local8Bit() ).stripWhiteSpace();
  }
  else {
    kdDebug() << "(K3bCdrecordProgram) could not start " << path << endl;
    return false;
  }

  if( !m_dvdPro && bin->version.suffix().endsWith( "-dvd" ) ) {
    bin->addFeature( "dvd-patch" );
    bin->version = QString(bin->version.versionString()).remove("-dvd");
  }

  // probe features
  KProcess fp;
  out.setProcess( &fp );
  fp << path << "-help";
  if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( out.output().contains( "gracetime" ) )
      bin->addFeature( "gracetime" );
    if( out.output().contains( "-overburn" ) )
      bin->addFeature( "overburn" );
    if( out.output().contains( "-text" ) )
      bin->addFeature( "cdtext" );
    if( out.output().contains( "-clone" ) )
      bin->addFeature( "clone" );
    if( out.output().contains( "-tao" ) )
      bin->addFeature( "tao" );
    if( out.output().contains( "cuefile=" ) && 
	( wodim || bin->version > K3bVersion( 2, 1, -1, "a14") ) ) // cuefile handling was still buggy in a14
      bin->addFeature( "cuefile" );

    // new mode 2 options since cdrecord 2.01a12
    // we use both checks here since the help was not updated in 2.01a12 yet (well, I 
    // just double-checked and the help page is proper but there is no harm in having 
    // two checks)
    // and the version check does not handle versions like 2.01-dvd properly
    if( out.output().contains( "-xamix" ) ||
	bin->version >= K3bVersion( 2, 1, -1, "a12" ) || 
	wodim )
      bin->addFeature( "xamix" );
   
    // check if we run cdrecord as root
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
  }
  else {
    kdDebug() << "(K3bCdrecordProgram) could not start " << bin->path << endl;
    delete bin;
    return false;
  }

  // FIXME: are these version correct?
  if( bin->version >= K3bVersion("1.11a38") || wodim )
    bin->addFeature( "plain-atapi" );
  if( bin->version > K3bVersion("1.11a17") || wodim )
    bin->addFeature( "hacked-atapi" );

  if( bin->version >= K3bVersion( 2, 1, 1, "a02" ) || wodim )
    bin->addFeature( "short-track-raw" );

  if( bin->version >= K3bVersion( 2, 1, -1, "a13" ) || wodim )
    bin->addFeature( "audio-stdin" );

  if( bin->version >= K3bVersion( "1.11a02" ) || wodim )
    bin->addFeature( "burnfree" );
  else
    bin->addFeature( "burnproof" );

  addBin( bin );
  return true;
}



K3bMkisofsProgram::K3bMkisofsProgram()
  : K3bExternalProgram( "mkisofs" )
{
}

bool K3bMkisofsProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("mkisofs");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path << "-version";
  K3bProcessOutputCollector out( &vp );
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "mkisofs" );
    if( pos < 0 )
      return false;

    pos = out.output().find( QRegExp("[0-9]"), pos );
    if( pos < 0 )
      return false;

    int endPos = out.output().find( ' ', pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bMkisofsProgram) could not start " << path << endl;
    return false;
  }



  // probe features
  KProcess fp;
  fp << path << "-help";
  out.setProcess( &fp );
  if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( out.output().contains( "-udf" ) )
      bin->addFeature( "udf" );
    if( out.output().contains( "-dvd-video" ) )
      bin->addFeature( "dvd-video" );
    if( out.output().contains( "-joliet-long" ) )
      bin->addFeature( "joliet-long" );
    if( out.output().contains( "-xa" ) )
      bin->addFeature( "xa" );
    if( out.output().contains( "-sectype" ) )
      bin->addFeature( "sectype" );

    // check if we run mkisofs as root
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
  }
  else {
    kdDebug() << "(K3bMkisofsProgram) could not start " << bin->path << endl;
    delete bin;
    return false;
  }

  addBin(bin);
  return true;
}


K3bReadcdProgram::K3bReadcdProgram()
  : K3bExternalProgram( "readcd" )
{
}

bool K3bReadcdProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("readcd");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path << "-version";
  K3bProcessOutputCollector out( &vp );
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "readcd" );
    if( pos < 0 )
      return false;

    pos = out.output().find( QRegExp("[0-9]"), pos );
    if( pos < 0 )
      return false;

    int endPos = out.output().find( ' ', pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bMkisofsProgram) could not start " << path << endl;
    return false;
  }



  // probe features
  KProcess fp;
  fp << path << "-help";
  out.setProcess( &fp );
  if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( out.output().contains( "-clone" ) )
      bin->addFeature( "clone" );

    // check if we run mkisofs as root
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
  }
  else {
    kdDebug() << "(K3bReadcdProgram) could not start " << bin->path << endl;
    delete bin;
    return false;
  }


  // FIXME: are these version correct?
  if( bin->version >= K3bVersion("1.11a38") )
    bin->addFeature( "plain-atapi" );
  if( bin->version > K3bVersion("1.11a17") )
    bin->addFeature( "hacked-atapi" );

  addBin(bin);
  return true;
}


K3bCdrdaoProgram::K3bCdrdaoProgram()
  : K3bExternalProgram( "cdrdao" )
{
}

bool K3bCdrdaoProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("cdrdao");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path ;
  K3bProcessOutputCollector out( &vp );
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "Cdrdao version" );
    if( pos < 0 )
      return false;

    pos = out.output().find( QRegExp("[0-9]"), pos );
    if( pos < 0 )
      return false;

    int endPos = out.output().find( ' ', pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );

    pos = out.output().find( "(C)", endPos+1 ) + 4;
    endPos = out.output().find( '\n', pos );
    bin->copyright = out.output().mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bCdrdaoProgram) could not start " << path << endl;
    return false;
  }



  // probe features
  KProcess fp;
  fp << path << "write" << "-h";
  out.setProcess( &fp );
  if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( out.output().contains( "--overburn" ) )
      bin->addFeature( "overburn" );
    if( out.output().contains( "--multi" ) )
      bin->addFeature( "multisession" );

    if( out.output().contains( "--buffer-under-run-protection" ) )
      bin->addFeature( "disable-burnproof" );

    // check if we run cdrdao as root
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
  }
  else {
    kdDebug() << "(K3bCdrdaoProgram) could not start " << bin->path << endl;
    delete bin;
    return false;
  }


  // SuSE 9.0 ships with a patched cdrdao 1.1.7 which contains an updated libschily
  // Gentoo ships with a patched cdrdao 1.1.7 which contains scglib support
  if( bin->version > K3bVersion( 1, 1, 7 ) || 
      bin->version == K3bVersion( 1, 1, 7, "-gentoo" ) ||
      bin->version == K3bVersion( 1, 1, 7, "-suse" ) ) {
    //    bin->addFeature( "plain-atapi" );
    bin->addFeature( "hacked-atapi" );
  }

  if( bin->version >= K3bVersion( 1, 1, 8 ) )
    bin->addFeature( "plain-atapi" );

  addBin(bin);
  return true;
}


K3bTranscodeProgram::K3bTranscodeProgram( const QString& transcodeProgram )
  : K3bExternalProgram( transcodeProgram ),
    m_transcodeProgram( transcodeProgram )
{
}

bool K3bTranscodeProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  if( path[path.length()-1] != '/' )
    path.append("/");

  QString appPath = path + m_transcodeProgram;

  if( !QFile::exists( appPath ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << appPath << "-v";
  K3bProcessOutputCollector out( &vp );
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "transcode v" );
    if( pos < 0 )
      return false;

    pos += 11;

    int endPos = out.output().find( QRegExp("[\\s\\)]"), pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = appPath;
    bin->version = out.output().mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bTranscodeProgram) could not start " << appPath << endl;
    return false;
  }

  //
  // Check features
  //
  QString modInfoBin = path + "tcmodinfo";
  KProcess modp;
  modp << modInfoBin << "-p";
  out.setProcess( &modp );
  if( modp.start( KProcess::Block, KProcess::AllOutput ) ) {
    QString modPath = out.output().stripWhiteSpace();
    QDir modDir( modPath );
    if( !modDir.entryList( "*export_xvid*", QDir::Files ).isEmpty() )
      bin->addFeature( "xvid" );
    if( !modDir.entryList( "*export_lame*", QDir::Files ).isEmpty() )
      bin->addFeature( "lame" );
    if( !modDir.entryList( "*export_ffmpeg*", QDir::Files ).isEmpty() )
      bin->addFeature( "ffmpeg" );
    if( !modDir.entryList( "*export_ac3*", QDir::Files ).isEmpty() )
      bin->addFeature( "ac3" );
  }

  addBin(bin);
  return true;
}



K3bVcdbuilderProgram::K3bVcdbuilderProgram( const QString& p )
  : K3bExternalProgram( p ),
    m_vcdbuilderProgram( p )
{
}

bool K3bVcdbuilderProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append(m_vcdbuilderProgram);
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path << "-V";
  K3bProcessOutputCollector out( &vp );
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "GNU VCDImager" );
    if( pos < 0 )
      return false;

    pos += 14;

    int endPos = out.output().find( QRegExp("[\\n\\)]"), pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos ).stripWhiteSpace();

    pos = out.output().find( "Copyright" ) + 14;
    endPos = out.output().find( "\n", pos );
    bin->copyright = out.output().mid( pos, endPos-pos ).stripWhiteSpace();
  }
  else {
    kdDebug() << "(K3bVcdbuilderProgram) could not start " << path << endl;
    return false;
  }

  addBin(bin);
  return true;
}


K3bNormalizeProgram::K3bNormalizeProgram()
  : K3bExternalProgram( "normalize" )
{
}


bool K3bNormalizeProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("normalize");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  K3bProcessOutputCollector out( &vp );

  vp << path << "--version";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "normalize" );
    if( pos < 0 )
      return false;

    pos = out.output().find( QRegExp("\\d"), pos );
    if( pos < 0 )
      return false;

    int endPos = out.output().find( QRegExp("\\s"), pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );

    pos = out.output().find( "Copyright" )+14;
    endPos = out.output().find( "\n", pos );
    bin->copyright = out.output().mid( pos, endPos-pos ).stripWhiteSpace();
  }
  else {
    kdDebug() << "(K3bCdrecordProgram) could not start " << path << endl;
    return false;
  }

  addBin( bin );
  return true;
}


K3bGrowisofsProgram::K3bGrowisofsProgram()
  : K3bExternalProgram( "growisofs" )
{
}

bool K3bGrowisofsProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("growisofs");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  K3bProcessOutputCollector out( &vp );

  vp << path << "-version";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "growisofs" );
    if( pos < 0 )
      return false;

    pos = out.output().find( QRegExp("\\d"), pos );
    if( pos < 0 )
      return false;

    int endPos = out.output().find( ",", pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bGrowisofsProgram) could not start " << path << endl;
    return false;
  }

  // fixed Copyright:
  bin->copyright = "Andy Polyakov <appro@fy.chalmers.se>";

  // check if we run growisofs as root
  struct stat s;
  if( !::stat( QFile::encodeName(path), &s ) ) {
    if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
      bin->addFeature( "suidroot" );
  }

  addBin( bin );
  return true;
}


K3bDvdformatProgram::K3bDvdformatProgram()
  : K3bExternalProgram( "dvd+rw-format" )
{
}

bool K3bDvdformatProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("dvd+rw-format");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  K3bProcessOutputCollector out( &vp );

  vp << path;
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    // different locales make searching for the +- char difficult
    // so we simply ignore it.
    int pos = out.output().find( QRegExp("DVD.*RW(/-RAM)? format utility") );
    if( pos < 0 )
      return false;

    pos = out.output().find( "version", pos );
    if( pos < 0 )
      return false;

    pos += 8;

    // the version ends in a dot.
    int endPos = out.output().find( QRegExp("\\.\\D"), pos );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bDvdformatProgram) could not start " << path << endl;
    return false;
  }

  // fixed Copyright:
  bin->copyright = "Andy Polyakov <appro@fy.chalmers.se>";

  // check if we run dvd+rw-format as root
  struct stat s;
  if( !::stat( QFile::encodeName(path), &s ) ) {
    if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
      bin->addFeature( "suidroot" );
  }
  
  addBin( bin );
  return true;
}


K3bDvdBooktypeProgram::K3bDvdBooktypeProgram()
  : K3bExternalProgram( "dvd+rw-booktype" )
{
}

bool K3bDvdBooktypeProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("dvd+rw-booktype");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  K3bProcessOutputCollector out( &vp );

  vp << path;
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "dvd+rw-booktype" );
    if( pos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    // No version information. Create dummy version
    bin->version = K3bVersion( 1, 0, 0 );
  }
  else {
    kdDebug() << "(K3bDvdBooktypeProgram) could not start " << path << endl;
    return false;
  }

  addBin( bin );
  return true;
}



K3bCdda2wavProgram::K3bCdda2wavProgram()
  : K3bExternalProgram( "cdda2wav" )
{
}

bool K3bCdda2wavProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("cdda2wav");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  K3bProcessOutputCollector out( &vp );

  vp << path << "-h";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "cdda2wav" );
    if( pos < 0 )
      return false;

    pos = out.output().find( "Version", pos );
    if( pos < 0 )
      return false;

    pos += 8;

    // the version does not end in a space but the kernel info
    int endPos = out.output().find( QRegExp("[^\\d\\.]"), pos );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );

    // features (we do this since the cdda2wav help says that the short
    //           options will disappear soon)
    if( out.output().find( "-info-only" ) )
      bin->addFeature( "info-only" ); // otherwise use the -J option
    if( out.output().find( "-no-infofile" ) )
      bin->addFeature( "no-infofile" ); // otherwise use the -H option
    if( out.output().find( "-gui" ) )
      bin->addFeature( "gui" ); // otherwise use the -g option
    if( out.output().find( "-bulk" ) )
      bin->addFeature( "bulk" ); // otherwise use the -B option
    if( out.output().find( "dev=" ) )
      bin->addFeature( "dev" ); // otherwise use the -B option
  }
  else {
    kdDebug() << "(K3bCdda2wavProgram) could not start " << path << endl;
    return false;
  }

  // check if we run as root
  struct stat s;
  if( !::stat( QFile::encodeName(path), &s ) ) {
    if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
      bin->addFeature( "suidroot" );
  }
 
  addBin( bin );
  return true;
}

