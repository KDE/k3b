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


#include "k3bdefaultexternalprograms.h"
#include "k3bexternalbinmanager.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qobject.h>
#include <qregexp.h>

#include <k3bprocess.h>
#include <kdebug.h>

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>



void K3b::addDefaultPrograms( K3bExternalBinManager* m )
{
  // don't know if we need more vcdTools in the future (vcdxrip)
  static const char* vcdTools[] =  { "vcdxbuild",
                                     "vcdxminfo",
                                     "vcdxrip",
                                     0 };
  
  static const char* transcodeTools[] =  { "transcode",
					   "tcprobe",
					   "tccat",
					   "tcscan",
					   "tcextract",
					   "tcdecode",
					   0 };
  for( int i = 0; vcdTools[i]; ++i )
    m->addProgram( new K3bVcdbuilderProgram( vcdTools[i] ) );
  for( int i = 0; transcodeTools[i]; ++i )
    m->addProgram( new K3bTranscodeProgram( transcodeTools[i] ) );
  m->addProgram( new K3bCdrecordProgram(false) );
  //  m->addProgram( new K3bCdrecordProgram(true) );
  //  m->addProgram( new K3bDvdrecordProgram() );
  m->addProgram( new K3bMkisofsProgram() );
  m->addProgram( new K3bReadcdProgram() );
  m->addProgram( new K3bCdrdaoProgram() );
  m->addProgram( new K3bNormalizeProgram() );
  m->addProgram( new K3bGrowisofsProgram() );
  m->addProgram( new K3bDvdformatProgram() );
//  m->addProgram( new K3bCdda2wavProgram() );
}


K3bCdrecordProgram::K3bCdrecordProgram( bool dvdPro )
  : K3bExternalProgram( dvdPro ? "cdrecord-prodvd" : "cdrecord" ),
    m_dvdPro(dvdPro)
{
}


bool K3bCdrecordProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("cdrecord");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  K3bProcess::OutputCollector out( &vp );

  vp << path << "-version";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = -1;
    if( m_dvdPro ) {
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

    pos = out.output().find( "Copyright") + 14;
    endPos = out.output().find( "\n", pos );
    bin->copyright = out.output().mid( pos, endPos-pos ).stripWhiteSpace();
  }
  else {
    kdDebug() << "(K3bCdrecordProgram) could not start " << path << endl;
    return false;
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
    if( out.output().contains( "-clone" ) )  // cdrecord ProDVD
      bin->addFeature( "clone" );
    if( out.output().contains( "cuefile=" ) && 
	bin->version > K3bVersion( 2, 1, -1, "a14") ) // cuefile handling was still buggy in a14
      bin->addFeature( "cuefile" );
    
    // check if we run cdrecord as root
    if( !getuid() )
      bin->addFeature( "suidroot" );
    else {
      struct stat s;
      if( !::stat( QFile::encodeName(path), &s ) ) {
	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	  bin->addFeature( "suidroot" );
      }
    }
  }
  else {
    kdDebug() << "(K3bCdrecordProgram) could not start " << bin->path << endl;
    delete bin;
    return false;
  }

  if( !m_dvdPro && bin->version.suffix().endsWith( "-dvd" ) )
    bin->addFeature( "dvd-patch" );

  // FIXME: are these version correct?
  if( bin->version >= K3bVersion("1.11a38") )
    bin->addFeature( "plain-atapi" );
  if( bin->version > K3bVersion("1.11a17") )
    bin->addFeature( "hacked-atapi" );

  addBin( bin );
  return true;
}


K3bDvdrecordProgram::K3bDvdrecordProgram()
  : K3bExternalProgram( "dvdrecord" )
{
}


bool K3bDvdrecordProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");
    path.append("dvdrecord");
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  K3bProcess::OutputCollector out( &vp );

  vp << path << "-version";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "dvdrtools" );
    if( pos < 0 )
      return false;

    pos = out.output().find( QRegExp("[0-9]"), pos );
    if( pos < 0 )
      return false;

    int endPos = out.output().find( "\n", pos );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bDvdrecordProgram) could not start " << path << endl;
    return false;
  }



  // probe features
  KProcess fp;
  out.setProcess( &fp );
  fp << path << "-help";
  if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( out.output().contains( "-delay" ) )
      bin->addFeature( "delay" );
    if( out.output().contains( "-overburn" ) )
      bin->addFeature( "overburn" );
    
    // check if we run cdrecord as root
    if( !getuid() )
      bin->addFeature( "suidroot" );
    else {
      struct stat s;
      if( !::stat( QFile::encodeName(path), &s ) ) {
	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	  bin->addFeature( "suidroot" );
      }
    }
  }
  else {
    kdDebug() << "(K3bDvdrecordProgram) could not start " << bin->path << endl;
    delete bin;
    return false;
  }

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
  K3bProcess::OutputCollector out( &vp );
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
    if( !getuid() )
      bin->addFeature( "suidroot" );
    else {
      struct stat s;
      if( !::stat( QFile::encodeName(path), &s ) ) {
	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	  bin->addFeature( "suidroot" );
      }
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
  K3bProcess::OutputCollector out( &vp );
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
    if( !getuid() )
      bin->addFeature( "suidroot" );
    else {
      struct stat s;
      if( !::stat( QFile::encodeName(path), &s ) ) {
	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	  bin->addFeature( "suidroot" );
      }
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
  K3bProcess::OutputCollector out( &vp );
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
    if( !getuid() )
      bin->addFeature( "suidroot" );
    else {
      struct stat s;
      if( !::stat( QFile::encodeName(path), &s ) ) {
	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	  bin->addFeature( "suidroot" );
      }
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
  path.append(m_transcodeProgram);

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path ;
  K3bProcess::OutputCollector out( &vp );
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "transcode v" );
    if( pos < 0 )
      return false;

    pos += 11;

    int endPos = out.output().find( QRegExp("[\\s\\)]"), pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.output().mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bTranscodeProgram) could not start " << path << endl;
    return false;
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
  K3bProcess::OutputCollector out( &vp );
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
  K3bProcess::OutputCollector out( &vp );

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
  K3bProcess::OutputCollector out( &vp );

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
  if( !getuid() )
    bin->addFeature( "suidroot" );
  else {
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
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
  K3bProcess::OutputCollector out( &vp );

  vp << path;
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "DVD±RW format utility" );
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
  if( !getuid() )
    bin->addFeature( "suidroot" );
  else {
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
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
  K3bProcess::OutputCollector out( &vp );

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
  if( !getuid() )
    bin->addFeature( "suidroot" );
  else {
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
  }

  addBin( bin );
  return true;
}

