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

#include <qfile.h>
#include <qtextstream.h>
#include <qdir.h>

#include <kdebug.h>
#include <klocale.h>


K3bMovixInstallation::K3bMovixInstallation( const QString& path )
  : m_path(path)
{
  if( m_path[m_path.length()-1] == '/' )
    m_path.truncate( m_path.length()-1 );
}


K3bMovixInstallation::~K3bMovixInstallation()
{
}


QString K3bMovixInstallation::subtitleFontDir( const QString& font ) const
{
  if( font == i18n("none" ) )
    return "";
  else if( m_supportedSubtitleFonts.contains( font ) )
    return m_path + "/mplayer-fonts/" + font;
  else
    return "";
}


QString K3bMovixInstallation::languageDir( const QString& lang ) const
{
  if( lang == i18n("default") )
    return languageDir( "en" );
  else if( m_supportedLanguages.contains( lang ) )
    return m_path + "/boot-messages/" + lang;
  else
    return "";
}


QStringList K3bMovixInstallation::isolinuxFiles()
{
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


QStringList K3bMovixInstallation::movixFiles()
{
  static QStringList s_movixFiles;
  if( s_movixFiles.isEmpty() ) {
    s_movixFiles.append( "bugReport.sh" );
    s_movixFiles.append( "input.conf" ); 
    s_movixFiles.append( "lircrc" );
    s_movixFiles.append( "manpage.txt" );
    s_movixFiles.append( "menu.conf" );
    s_movixFiles.append( "mixer.pl" );
    s_movixFiles.append( "movix.pl" );
    s_movixFiles.append( "profile" );
    s_movixFiles.append( "rc.movix" );
  }

  return s_movixFiles;
}


K3bMovixInstallation* K3bMovixInstallation::probeInstallation( const QString& path )
{
  // first check if all necessary directories are present
  QDir dir(path);
  QStringList subdirs = dir.entryList();

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
    if( !QFile::exists( path + "/isolinux/" + *it ) ) {
      kdDebug() << "(K3bMovixInstallation) Could not find file " << *it << endl;
      return 0;
    }
  }
  QStringList movixFiles = K3bMovixInstallation::movixFiles();
  for( QStringList::const_iterator it = movixFiles.begin();
       it != movixFiles.end(); ++it ) {
    if( !QFile::exists( path + "/movix/" + *it ) ) {
      kdDebug() << "(K3bMovixInstallation) Could not find file " << *it << endl;
      return 0;
    }
  }



  // now check the boot-messages languages
  K3bMovixInstallation* inst = new K3bMovixInstallation( path );
  dir.cd( "boot-messages" );
  inst->m_supportedLanguages = dir.entryList(QDir::Dirs);
  inst->m_supportedLanguages.remove(".");
  inst->m_supportedLanguages.remove("..");
  inst->m_supportedLanguages.remove("CVS");  // the eMovix makefile stuff seems not perfect ;)
  inst->m_supportedLanguages.prepend( i18n("default") );
  dir.cdUp();

  // now check the supported mplayer-fontsets
  dir.cd( "mplayer-fonts" );
  inst->m_supportedSubtitleFonts = dir.entryList( QDir::Dirs );
  inst->m_supportedSubtitleFonts.remove(".");
  inst->m_supportedSubtitleFonts.remove("..");
  inst->m_supportedSubtitleFonts.remove("CVS");  // the eMovix makefile stuff seems not perfect ;)
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
