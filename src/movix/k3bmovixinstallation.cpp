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


#include "k3bmovixinstallation.h"

#include <qfile.h>
#include <qtextstream.h>
#include <qdir.h>

#include <kdebug.h>


K3bMovixInstallation::K3bMovixInstallation( const QString& path )
  : m_path(path)
{
}


K3bMovixInstallation::~K3bMovixInstallation()
{
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

  // TODO: check every single necessary file :(

  // now check the boot-messages languages
  K3bMovixInstallation* inst = new K3bMovixInstallation( path );
  dir.cd( "boot-messages" );
  inst->m_supportedLanguages = dir.entryList(QDir::Dirs);
  inst->m_supportedLanguages.remove(".");
  inst->m_supportedLanguages.remove("..");
  dir.cdUp();

  // now check the supported mplayer-fontsets
  dir.cd( "mplayer-fonts" );
  inst->m_supportedSubtitleFonts = dir.entryList( QDir::Dirs );
  inst->m_supportedSubtitleFonts.remove(".");
  inst->m_supportedSubtitleFonts.remove("..");
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
  
  return inst;
}
