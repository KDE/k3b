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

#include "k3bthememanager.h"

#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <ksimpleconfig.h>
#include <kdebug.h>
#include <kglobal.h>

#include <qpixmap.h>
#include <qfile.h>
#include <qdir.h>
#include <qstringlist.h>



K3bThemeManager* K3bThemeManager::s_k3bThemeManager = 0;

const QPixmap& K3bTheme::pixmap( const QString& name ) const
{
  QMap<QString, QPixmap>::const_iterator it = m_pixmapMap.find( name );
  if( it != m_pixmapMap.end() )
    return *it;

  // try loading the image
  if( QFile::exists( m_path + name + ".png" ) )
    return *m_pixmapMap.insert( name, QPixmap( m_path + name + ".png" ) );
    
  kdDebug() << "(K3bTheme) " << m_name << ": could not load image " << name << endl;

  return m_emptyPixmap;
}



class K3bThemeManager::Private
{
public:
  Private()
    : currentTheme(0) {
  }

  QPtrList<K3bTheme> themes;
  K3bTheme* currentTheme;
  QString currentThemeName;
};



K3bThemeManager::K3bThemeManager( QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private();
  s_k3bThemeManager = this;
}


K3bThemeManager::~K3bThemeManager()
{
  delete d;
}


const QPtrList<K3bTheme>& K3bThemeManager::themes() const
{
  return d->themes;
}


K3bTheme* K3bThemeManager::currentTheme() const
{
  return d->currentTheme;
}


void K3bThemeManager::readConfig( KConfig* c )
{
  c->setGroup( "General Options" );
  setCurrentTheme( c->readEntry( "current theme" ) );
}


void K3bThemeManager::saveConfig( KConfig* c )
{
  c->setGroup( "General Options" );
  if( !d->currentThemeName.isEmpty() )
    c->writeEntry( "current theme", d->currentThemeName );
}


void K3bThemeManager::setCurrentTheme( const QString& name )
{
  if( name != d->currentThemeName ) {
    if( K3bTheme* theme = findTheme( name ) )
      setCurrentTheme( theme );
    else
      setCurrentTheme( d->themes.first() );
  }
}


void K3bThemeManager::setCurrentTheme( K3bTheme* theme )
{
  if( theme != d->currentTheme ) {
    d->currentTheme = theme;
    if( theme )
      d->currentThemeName = theme->name();
    else
      d->currentThemeName = QString::null;
    
    emit themeChanged();
    emit themeChanged( theme );
  }
}


K3bTheme* K3bThemeManager::findTheme( const QString& name ) const
{
  for( QPtrListIterator<K3bTheme> it( d->themes ); it.current(); ++it )
    if( it.current()->name() == name )
      return it.current();
  return 0;
}


void K3bThemeManager::loadThemes()
{
  // first we cleanup the loaded themes
  d->themes.setAutoDelete(true);
  d->themes.clear();

  QStringList dirs = KGlobal::dirs()->findDirs( "data", "k3b/pics" );
  // now search for themes. As there may be multible themes with the same name
  // we only use the names from this list and then use findResourceDir to make sure
  // the local is preferred over the global stuff (like testing a theme by copying it
  // to the .kde dir)
  QStringList themeNames;
  for( QStringList::const_iterator dirIt = dirs.begin(); dirIt != dirs.end(); ++dirIt ) {
    QDir dir( *dirIt );
    QStringList entries = dir.entryList( QDir::Dirs );
    entries.remove( "." );
    entries.remove( ".." );
    // every theme dir needs to contain a k3b.theme file
    for( QStringList::const_iterator entryIt = entries.begin(); entryIt != entries.end(); ++entryIt )
      if( QFile::exists( *dirIt + *entryIt + "/k3b.theme" ) )
	themeNames.append( *entryIt );
  }

  // now load the themes
  for( QStringList::const_iterator themeIt = themeNames.begin(); themeIt != themeNames.end(); ++themeIt )
    loadTheme( *themeIt );

  // load the current theme
  setCurrentTheme( findTheme(d->currentThemeName) );
}


void K3bThemeManager::loadTheme( const QString& name )
{
  QString path = KGlobal::dirs()->findResource( "data", "k3b/pics/" + name + "/k3b.theme" );
  if( !path.isEmpty() ) {
    K3bTheme* t = new K3bTheme();
    t->m_name = name;
    t->m_path = path.left( path.length() - 9 );

    // load the stuff
    KSimpleConfig cfg( path, true );
    t->m_author = cfg.readEntry( "Author" );
    t->m_comment = cfg.readEntry( "Comment" );
    t->m_version = cfg.readEntry( "Version" );
    t->m_bgColor = KGlobalSettings::activeTitleColor();
    t->m_fgColor = KGlobalSettings::activeTextColor();
    t->m_bgColor = cfg.readColorEntry( "Backgroundcolor", &t->m_bgColor );
    t->m_fgColor = cfg.readColorEntry( "Foregroundcolor", &t->m_fgColor );

    d->themes.append( t );
  }
}


#include "k3bthememanager.moc"
