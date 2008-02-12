/*
 *
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

#include <kdebug.h>
#include <kprocess.h>

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

class K3bExternalScanner : public KProcess
{
public:
	K3bExternalScanner( QObject *parent = 0 );

	QString getVersion(const int &pos) const;
	QByteArray getData() const { return m_data; }

	bool run();

private:
	QByteArray m_data;
};

K3bExternalScanner::K3bExternalScanner( QObject *parent )
	: KProcess( parent )
{
	setOutputChannelMode( MergedChannels );
}

bool K3bExternalScanner::run()
{
	start();
	if( waitForFinished( -1 ) ) {
		m_data = readAll();
		return true;
	} else
		return false;
}

QString K3bExternalScanner::getVersion(const int &pos) const
{
	QString tmp = m_data;

	int sPos = tmp.indexOf( QRegExp("\\d"), pos );
	if( sPos < 0 )
		return QString();

	int endPos = tmp.indexOf( QRegExp("\\s"), sPos + 1 );
	if( endPos < 0 )
		return QString();

	return tmp.mid( sPos, endPos - sPos );
}

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
    kDebug() << "(K3bCdrecordProgram) checking for Debian cdrecord wrapper script.";
    if( QFileInfo( path ).size() < 1024 ) {
      kDebug() << "(K3bCdrecordProgram) Debian Wrapper script size fits. Checking file.";
      QFile f( path );
      f.open( QIODevice::ReadOnly );
      QString s = QTextStream( &f ).read();
      if( s.contains( "cdrecord.mmap" ) && s.contains( "cdrecord.shm" ) ) {
	kDebug() << "(K3bCdrecordProgram) Found Debian Wrapper script.";
	QString ext;
	if( K3b::kernelVersion().versionString().left(3) > "2.2" )
	  ext = ".mmap";
	else
	  ext = ".shm";

	kDebug() << "(K3bCdrecordProgram) Using cdrecord" << ext;

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

    if( QFile::exists( path + "wodim" ) ) {
      wodim = true;
      path += "wodim";
    }
    else if( QFile::exists( path + "cdrecord" ) ) {
      path += "cdrecord";
    }
    else
      return false;
  }

  debianWeirdnessHack( path );

  K3bExternalBin* bin = 0;

  // probe version
  K3bExternalScanner vp;
  vp << path << "-version";

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = -1;
    if( wodim ) {
      pos = out.indexOf( "Wodim" );
    }
    else if( m_dvdPro ) {
      pos = out.indexOf( "Cdrecord-ProDVD" );
    }
    else {
      pos = out.indexOf( "Cdrecord" );
    }

    if( pos < 0 )
      return false;

    QString ver = vp.getVersion( pos );
    if (ver.isEmpty())
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = ver;

    if( wodim )
      bin->addFeature( "wodim" );

    pos = out.indexOf( "Copyright") + 14;
    int endPos = out.indexOf( "\n", pos );

    // cdrecord does not use local encoding for the copyright statement but plain latin1
    bin->copyright = QString::fromLatin1( out.mid( pos, endPos-pos ).toLocal8Bit() ).trimmed();
  }
  else {
    kDebug() << "(K3bCdrecordProgram) could not start " << path;
    return false;
  }

  if( !m_dvdPro && bin->version.suffix().endsWith( "-dvd" ) ) {
    bin->addFeature( "dvd-patch" );
    bin->version = QString(bin->version.versionString()).remove("-dvd");
  }

  // probe features
  K3bExternalScanner fp;
  fp << path << "-help";

  if( fp.run() ) {
    QByteArray out = fp.getData();

    if( out.contains( "gracetime" ) )
      bin->addFeature( "gracetime" );
    if( out.contains( "-overburn" ) )
      bin->addFeature( "overburn" );
    if( out.contains( "-text" ) )
      bin->addFeature( "cdtext" );
    if( out.contains( "-clone" ) )
      bin->addFeature( "clone" );
    if( out.contains( "-tao" ) )
      bin->addFeature( "tao" );
    if( out.contains( "cuefile=" ) &&
	( wodim || bin->version > K3bVersion( 2, 1, -1, "a14") ) ) // cuefile handling was still buggy in a14
      bin->addFeature( "cuefile" );

    // new mode 2 options since cdrecord 2.01a12
    // we use both checks here since the help was not updated in 2.01a12 yet (well, I
    // just double-checked and the help page is proper but there is no harm in having
    // two checks)
    // and the version check does not handle versions like 2.01-dvd properly
    if( out.contains( "-xamix" ) ||
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
    kDebug() << "(K3bCdrecordProgram) could not start " << bin->path;
    delete bin;
    return false;
  }

  if( bin->version < K3bVersion( 2, 0 ) && !wodim )
    bin->addFeature( "outdated" );

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

  // FIXME: cdrecord Blu-ray support not 100% yet
//   if ( bin->version >= K3bVersion( 2, 1, 1, "a29" ) && !wodim )
//       bin->addFeature( "blu-ray" );

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

  bool genisoimage = false;
  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");

    if( QFile::exists( path + "genisoimage" ) ) {
      genisoimage = true;
      path += "genisoimage";
    }
    else if( QFile::exists( path + "mkisofs" ) ) {
      path += "mkisofs";
    }
    else
      return false;
  }

  K3bExternalBin* bin = 0;

  // probe version
  K3bExternalScanner vp;
  vp << path << "-version";

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = -1;
    if( genisoimage )
      pos = out.indexOf( "genisoimage" );
    else
      pos = out.indexOf( "mkisofs" );

    if( pos < 0 )
      return false;

    QString ver = vp.getVersion( pos );
    if( ver.isEmpty() )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = ver;

    if( genisoimage )
      bin->addFeature( "genisoimage" );
  }
  else {
    kDebug() << "(K3bMkisofsProgram) could not start " << path;
    return false;
  }



  // probe features
  K3bExternalScanner fp;
  fp << path << "-help";

  if( fp.run() ) {
    QByteArray out = fp.getData();
    if( out.contains( "-udf" ) )
      bin->addFeature( "udf" );
    if( out.contains( "-dvd-video" ) )
      bin->addFeature( "dvd-video" );
    if( out.contains( "-joliet-long" ) )
      bin->addFeature( "joliet-long" );
    if( out.contains( "-xa" ) )
      bin->addFeature( "xa" );
    if( out.contains( "-sectype" ) )
      bin->addFeature( "sectype" );

    // check if we run mkisofs as root
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
  }
  else {
    kDebug() << "(K3bMkisofsProgram) could not start " << bin->path;
    delete bin;
    return false;
  }

  if( bin->version < K3bVersion( 1, 14) && !genisoimage )
    bin->addFeature( "outdated" );

  if( bin->version >= K3bVersion( 1, 15, -1, "a40" ) || genisoimage )
    bin->addFeature( "backslashed_filenames" );

  if ( genisoimage && bin->version >= K3bVersion( 1, 1, 4 ) )
      bin->addFeature( "no-4gb-limit" );

  if ( !genisoimage && bin->version >= K3bVersion( 2, 1, 1, "a32" ) )
      bin->addFeature( "no-4gb-limit" );

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

  bool readom = false;
  QString path = p;
  QFileInfo fi( path );
  if( fi.isDir() ) {
    if( path[path.length()-1] != '/' )
      path.append("/");

    if( QFile::exists( path + "readom" ) ) {
      readom = true;
      path += "readom";
    }
    else if( QFile::exists( path + "readcd" ) ) {
      path += "readcd";
    }
    else
      return false;
  }

  if( !QFile::exists( path ) )
    return false;

  K3bExternalBin* bin = 0;

  // probe version
  K3bExternalScanner vp;
  vp << path << "-version";

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = -1;
    if( readom )
      pos = out.indexOf( "readom" );
    else
      pos = out.indexOf( "readcd" );
    if( pos < 0 )
      return false;

    QString ver = vp.getVersion( pos );
    if( ver.isEmpty() )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = ver;

    if( readom )
      bin->addFeature( "readom" );
  }
  else {
    kDebug() << "(K3bMkisofsProgram) could not start " << path;
    return false;
  }



  // probe features
  K3bExternalScanner fp;
  fp << path << "-help";

  if( fp.run() ) {
    QByteArray out = fp.getData();
    if( out.contains( "-clone" ) )
      bin->addFeature( "clone" );

    // check if we run mkisofs as root
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
  }
  else {
    kDebug() << "(K3bReadcdProgram) could not start " << bin->path;
    delete bin;
    return false;
  }


  // FIXME: are these version correct?
  if( bin->version >= K3bVersion("1.11a38") || readom )
    bin->addFeature( "plain-atapi" );
  if( bin->version > K3bVersion("1.11a17") || readom )
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
  K3bExternalScanner vp;
  vp << path ;

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = out.indexOf( "Cdrdao version" );
    if( pos < 0 )
      return false;

    QString ver = vp.getVersion( pos );
    if( ver.isEmpty() )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = ver;

    int endPos = out.indexOf( QRegExp("[0-9]"), pos );
    endPos = out.indexOf( QRegExp("\\s"), endPos + 1 );
    pos = out.indexOf( "(C)", endPos+1 ) + 4;
    endPos = out.indexOf( '\n', pos );
    bin->copyright = out.mid( pos, endPos-pos );
  }
  else {
    kDebug() << "(K3bCdrdaoProgram) could not start " << path;
    return false;
  }



  // probe features
  K3bExternalScanner fp;
  fp << path << "write" << "-h";

  if( fp.run() ) {
    QByteArray out = fp.getData();
    if( out.contains( "--overburn" ) )
      bin->addFeature( "overburn" );
    if( out.contains( "--multi" ) )
      bin->addFeature( "multisession" );

    if( out.contains( "--buffer-under-run-protection" ) )
      bin->addFeature( "disable-burnproof" );

    // check if we run cdrdao as root
    struct stat s;
    if( !::stat( QFile::encodeName(path), &s ) ) {
      if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	bin->addFeature( "suidroot" );
    }
  }
  else {
    kDebug() << "(K3bCdrdaoProgram) could not start " << bin->path;
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
  K3bExternalScanner vp;
  vp << appPath << "-v";

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = out.indexOf( "transcode v" );
    if( pos < 0 )
      return false;

    pos += 11;

    int endPos = out.indexOf( QRegExp("[\\s\\)]"), pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = appPath;
    bin->version = out.mid( pos, endPos-pos );
  }
  else {
    kDebug() << "(K3bTranscodeProgram) could not start " << appPath;
    return false;
  }

  //
  // Check features
  //
  QString modInfoBin = path + "tcmodinfo";
  K3bExternalScanner modp;
  modp << modInfoBin << "-p";

  if( modp.run() ) {
    QString modPath = modp.getData();
    QDir modDir( modPath );
    if( !modDir.entryList( QStringList() << "*export_xvid*", QDir::Files ).isEmpty() )
      bin->addFeature( "xvid" );
    if( !modDir.entryList( QStringList() << "*export_lame*", QDir::Files ).isEmpty() )
      bin->addFeature( "lame" );
    if( !modDir.entryList( QStringList() << "*export_ffmpeg*", QDir::Files ).isEmpty() )
      bin->addFeature( "ffmpeg" );
    if( !modDir.entryList( QStringList() << "*export_ac3*", QDir::Files ).isEmpty() )
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
  K3bExternalScanner vp;
  vp << path << "-V";

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = out.indexOf( "GNU VCDImager" );
    if( pos < 0 )
      return false;

    pos += 14;

    int endPos = out.indexOf( QRegExp("[\\n\\)]"), pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.mid( pos, endPos-pos ).trimmed();

    pos = out.indexOf( "Copyright" ) + 14;
    endPos = out.indexOf( "\n", pos );
    bin->copyright = out.mid( pos, endPos-pos ).trimmed();
  }
  else {
    kDebug() << "(K3bVcdbuilderProgram) could not start " << path;
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
  K3bExternalScanner vp;
  vp << path << "--version";

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = out.indexOf( "normalize" );
    if( pos < 0 )
      return false;

    QString ver = vp.getVersion( pos );
    if( ver.isEmpty() )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = ver;

    pos = out.indexOf( "Copyright" )+14;
    int endPos = out.indexOf( "\n", pos );
    bin->copyright = out.mid( pos, endPos-pos ).trimmed();
  }
  else {
    kDebug() << "(K3bCdrecordProgram) could not start " << path;
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
  K3bExternalScanner vp;
  vp << path << "-version";

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = out.indexOf( "growisofs" );
    if( pos < 0 )
      return false;

    pos = out.indexOf( QRegExp("\\d"), pos );
    if( pos < 0 )
      return false;

    int endPos = out.indexOf( ',', pos+1 );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.mid( pos, endPos-pos );
  }
  else {
    kDebug() << "(K3bGrowisofsProgram) could not start " << path;
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

  if ( bin->version >= K3bVersion( 5, 20 ) )
      bin->addFeature( "dual-layer" );

  if ( bin->version > K3bVersion( 5, 17 ) )
      bin->addFeature( "tracksize" );

  if ( bin->version >= K3bVersion( 5, 15 ) )
      bin->addFeature( "daosize" );

  if ( bin->version >= K3bVersion( 6, 0 ) )
      bin->addFeature( "buffer" );

  if ( bin->version >= K3bVersion( 7, 0 ) )
      bin->addFeature( "blu-ray" );

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
  K3bExternalScanner vp;
  vp << path;

  if( vp.run() ) {
    // different locales make searching for the +- char difficult
    // so we simply ignore it.
    QString out = vp.getData();
    int pos = out.indexOf( QRegExp("DVD.*RW(/-RAM)? format utility") );
    if( pos < 0 )
      return false;

    pos = out.indexOf( "version", pos );
    if( pos < 0 )
      return false;

    pos += 8;

    // the version ends in a dot.
    int endPos = out.indexOf( QRegExp("\\.\\D"), pos );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.mid( pos, endPos-pos );
  }
  else {
    kDebug() << "(K3bDvdformatProgram) could not start " << path;
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
  K3bExternalScanner vp;
  vp << path;

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = out.indexOf( "dvd+rw-booktype" );
    if( pos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    // No version information. Create dummy version
    bin->version = K3bVersion( 1, 0, 0 );
  }
  else {
    kDebug() << "(K3bDvdBooktypeProgram) could not start " << path;
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
  K3bExternalScanner vp;
  vp << path << "-h";

  if( vp.run() ) {
    QString out = vp.getData();
    int pos = out.indexOf( "cdda2wav" );
    if( pos < 0 )
      return false;

    pos = out.indexOf( "Version", pos );
    if( pos < 0 )
      return false;

    pos += 8;

    // the version does not end in a space but the kernel info
    int endPos = out.indexOf( QRegExp("[^\\d\\.]"), pos );
    if( endPos < 0 )
      return false;

    bin = new K3bExternalBin( this );
    bin->path = path;
    bin->version = out.mid( pos, endPos-pos );

    // features (we do this since the cdda2wav help says that the short
    //           options will disappear soon)
    if( out.indexOf( "-info-only" ) )
      bin->addFeature( "info-only" ); // otherwise use the -J option
    if( out.indexOf( "-no-infofile" ) )
      bin->addFeature( "no-infofile" ); // otherwise use the -H option
    if( out.indexOf( "-gui" ) )
      bin->addFeature( "gui" ); // otherwise use the -g option
    if( out.indexOf( "-bulk" ) )
      bin->addFeature( "bulk" ); // otherwise use the -B option
    if( out.indexOf( "dev=" ) )
      bin->addFeature( "dev" ); // otherwise use the -B option
  }
  else {
    kDebug() << "(K3bCdda2wavProgram) could not start " << path;
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
