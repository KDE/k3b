/***************************************************************************
 *   Copyright (C) 2002 by Sebastian Trueg                                 *
 *   trueg@k3b.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "k3bdefaultexternalprograms.h"
#include "k3bexternalbinmanager.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qobject.h>
#include <qregexp.h>

#include <kprocess.h>
#include <kdebug.h>

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>



void addDefaultPrograms( K3bExternalBinManager* m )
{
  static const char* vcdTools[] =  { "vcdxgen",
				     "vcdxbuild",
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
  m->addProgram( new K3bCdrecordProgram() );
  m->addProgram( new K3bMkisofsProgram() );
  m->addProgram( new K3bCdrdaoProgram() );
  m->addProgram( new K3bEMovixProgram() );
}


K3bCdrecordProgram::K3bCdrecordProgram()
  : K3bExternalProgram( "cdrecord" )
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
  OutputCollector out( &vp );

  vp << path << "-version";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = out.output().find( "Cdrecord" );
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
  OutputCollector out( &vp );
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
  OutputCollector out( &vp );
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
  out.setProcess( &vp );
  if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( out.output().contains( "--overburn" ) )
      bin->addFeature( "overburn" );
    if( out.output().contains( "--multi" ) )
      bin->addFeature( "multisession" );

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
  OutputCollector out( &vp );
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


K3bEMovixProgram::K3bEMovixProgram()
  : K3bExternalProgram( "eMovix" )
{
}

bool K3bEMovixProgram::scan( const QString& p )
{
  if( p.isEmpty() )
    return false;

  QString path = p;
  if( path[path.length()-1] != '/' )
    path.append("/");
  path.append("movix-version");

  // first test if we have a version info (eMovix >= 0.8.0pre3)
  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path;
  OutputCollector out( &vp );
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    // movix-version just gives us the version number on stdout
    if( !out.output().isEmpty() ) {
      bin = new K3bExternalBin( this );
      bin->version = out.output().stripWhiteSpace();
    }
  }
  else {
    kdDebug() << "(K3bEMovixProgram) could not start " << path << endl;
    return false;
  }


  path = p;
  if( path[path.length()-1] != '/' )
    path.append("/");
  path.append("movix-conf");

  // now search for the config and the files
  if( !QFile::exists( path ) ) {
    delete bin;
    return false;
  }

  KProcess cp;
  cp << path;
  out.setProcess( &cp );
  if( cp.start( KProcess::Block, KProcess::AllOutput ) ) {
    // now search the needed files in the given dir
    if( out.output().isEmpty() ) {
      kdDebug() << "(K3bEMovixProgram) no eMovix config info" << endl;
      delete bin;
      return false;
    }

    // we need the following files:
    // isolinux/initrd.gz
    // isolinux/iso.sort
    // isolinux/isolinux.bin
    // isolinux/isolinux.cfg
    // isolinux/movix.lss
    // isolinux/mphelp.txt
    // isolinux/mxhelp.txt
    // isolinux/trblst.txt
    // isolinux/credits.txt
    // isolinux/kernel/vmlinuz

    // TODO: search the files

    // the eMovix bin does not contain the path to any executable but the path to the eMovix files
    bin->path = out.output().stripWhiteSpace();
    addBin(bin);
    return true;
  }
  else {
    kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
    delete bin;
    return false;
  }
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
  OutputCollector out( &vp );
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
  }
  else {
    kdDebug() << "(K3bVcdbuilderProgram) could not start " << path << endl;
    return false;
  }

  addBin(bin);
  return true;
}
