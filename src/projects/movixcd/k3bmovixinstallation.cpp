/* 
 *
 * $Id$
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


#include "k3bmovixinstallation.h"

#include <k3bexternalbinmanager.h>
#include <k3bprocess.h>

#include <qfile.h>
#include <qtextstream.h>
#include <qdir.h>

#include <kdebug.h>
#include <klocale.h>


K3bMovixInstallation::K3bMovixInstallation( const K3bExternalBin* bin )
  : m_bin(bin)
{
}


K3bMovixInstallation::~K3bMovixInstallation()
{
}


QString K3bMovixInstallation::subtitleFontDir( const QString& font ) const
{
  if( font == i18n("none" ) )
    return "";
  else if( m_supportedSubtitleFonts.contains( font ) )
    return path() + "/mplayer-fonts/" + font;
  else
    return "";
}


QString K3bMovixInstallation::languageDir( const QString& lang ) const
{
  if( lang == i18n("default") )
    return languageDir( "en" );
  else if( m_supportedLanguages.contains( lang ) )
    return path() + "/boot-messages/" + lang;
  else
    return "";
}


QStringList K3bMovixInstallation::isolinuxFiles()
{
  // these files are fixed. That should not be a problem
  // since Isolinux is quite stable as far as I know.

  static QStringList s_isolinuxFiles;
  if( s_isolinuxFiles.isEmpty() ) {
    s_isolinuxFiles.append( "initrd.gz" );
    s_isolinuxFiles.append( "isolinux.bin" );
    s_isolinuxFiles.append( "isolinux.cfg" );
    s_isolinuxFiles.append( "kernel/vmlinuz" );
    s_isolinuxFiles.append( "movix.lss" );
    s_isolinuxFiles.append( "movix.msg" );
  }

  return s_isolinuxFiles;
}


const QStringList& K3bMovixInstallation::movixFiles()
{
  // we cache the movix files
  if( m_movixFiles.isEmpty() ) {
    if( m_bin->hasFeature( "files" ) ) {
      KProcess p;
      K3bProcess::OutputCollector out( &p );
      p << m_bin->path + "movix-files";
      if( p.start( KProcess::Block, KProcess::AllOutput ) ) {
	m_movixFiles = QStringList::split( "\n", out.output() );
      }
    }
    
    else {
      // fallback; to be compatible with 0.8.0rc2 we just add all files in the movix directory
      QDir dir( path() + "/movix" );
      m_movixFiles = dir.entryList(QDir::Files);
    }
  }

  return m_movixFiles;
}


K3bMovixInstallation* K3bMovixInstallation::probeInstallation( const K3bExternalBin* bin )
{
  // we first need to get the movix dir
  QString movixPath;
  KProcess cp;
  K3bProcess::OutputCollector out( &cp );
  cp << bin->path + "movix-conf";
  if( cp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( out.output().isEmpty() ) {
      kdDebug() << "(K3bMovixInstallation) no eMovix config info" << endl;
      return 0;
    }

    movixPath = out.output().stripWhiteSpace();
  }
  else {
    kdDebug() << "(K3bMovixInstallation) could not start " << bin->path << "movix-conf" << endl;
    return 0;
  }

  // first check if all necessary directories are present
  QDir dir(movixPath);
  kdDebug() << "(K3bMovixInstallation) searching for emovix files in '" << dir.path() << "'" << endl;
  QStringList subdirs = dir.entryList( QDir::Dirs );

  if( !subdirs.contains( "boot-messages" ) ) {
    kdDebug() << "(K3bMovixInstallation) could not find subdir 'boot-messages'" << endl;
    return 0;
  }
  if( !subdirs.contains( "isolinux" ) ) {
    kdDebug() << "(K3bMovixInstallation) could not find subdir 'isolinux'" << endl;
    return 0;
  }
  if( !subdirs.contains( "movix" ) ) {
    kdDebug() << "(K3bMovixInstallation) could not find subdir 'movix'" << endl;
    return 0;
  }
  if( !subdirs.contains( "mplayer-fonts" ) ) {
    kdDebug() << "(K3bMovixInstallation) could not find subdir 'mplayer-fonts'" << endl;
    return 0;
  }

  // ok, all subdirs present

  // check every single necessary file :(

  QStringList isolinuxFiles = K3bMovixInstallation::isolinuxFiles();
  for( QStringList::const_iterator it = isolinuxFiles.begin();
       it != isolinuxFiles.end(); ++it ) {
    if( !QFile::exists( movixPath + "/isolinux/" + *it ) ) {
      kdDebug() << "(K3bMovixInstallation) Could not find file " << *it << endl;
      return 0;
    }
  }
//   QStringList movixFiles = K3bMovixInstallation::movixFiles();
//   for( QStringList::const_iterator it = movixFiles.begin();
//        it != movixFiles.end(); ++it ) {
//     if( !QFile::exists( path + "/movix/" + *it ) ) {
//       kdDebug() << "(K3bMovixInstallation) Could not find file " << *it << endl;
//       return 0;
//     }
//   }



  // now check the boot-messages languages
  K3bMovixInstallation* inst = new K3bMovixInstallation( bin );
  inst->m_movixPath = movixPath;
  dir.cd( "boot-messages" );
  inst->m_supportedLanguages = dir.entryList(QDir::Dirs);
  inst->m_supportedLanguages.remove(".");
  inst->m_supportedLanguages.remove("..");
  inst->m_supportedLanguages.remove("CVS");  // the eMovix makefile stuff seems not perfect ;)
  inst->m_supportedLanguages.prepend( i18n("default") );
  dir.cdUp();

  // now check the supported mplayer-fontsets
  // FIXME: every font dir needs to contain the "font.desc" file!
  dir.cd( "mplayer-fonts" );
  inst->m_supportedSubtitleFonts = dir.entryList( QDir::Dirs );
  inst->m_supportedSubtitleFonts.remove(".");
  inst->m_supportedSubtitleFonts.remove("..");
  inst->m_supportedSubtitleFonts.remove("CVS");  // the eMovix makefile stuff seems not perfect ;)
  // new ttf fonts in 0.8.0rc2
  inst->m_supportedSubtitleFonts += dir.entryList( "*.ttf", QDir::Files );
  inst->m_supportedSubtitleFonts.prepend( i18n("none") );
  dir.cdUp();
  
  // now check the supported boot labels
  dir.cd( "isolinux" );
  QFile f( dir.filePath("isolinux.cfg") );
  if( !f.open( IO_ReadOnly ) ) {
    kdDebug() << "(K3bMovixInstallation) could not open file '" << f.name() << "'" << endl;
    delete inst;
    return 0;
  }
  QTextStream fs( &f );
  QString line = fs.readLine();
  while( !line.isNull() ) {
    if( line.startsWith( "label" ) ) {
      inst->m_supportedBootLabels.append( line.mid( 5 ).stripWhiteSpace() );
    }
    line = fs.readLine();
  }
  f.close();
  inst->m_supportedBootLabels.prepend( i18n("default") );
  
  return inst;
}
