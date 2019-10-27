/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bthememanager.h"

#include "k3bversion.h"

#include <KConfigGroup>
#include <KColorScheme>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>
#include <QPixmap>


K3b::Theme::Theme()
    : m_bgMode(BG_TILE)
{
}


K3b::Theme::Theme( QString name)
    : m_bgMode(BG_TILE)
{
    QString path = QStandardPaths::locate( QStandardPaths::GenericDataLocation, "k3b/pics/" + name + "/k3b.theme" );
    if( !path.isEmpty() )
        m_path = path.left( path.length() - 9 );
}


QColor K3b::Theme::backgroundColor() const
{
    if( m_bgColor.isValid() )
        return m_bgColor;
    else
        return KColorScheme(QPalette::Active, KColorScheme::Window).background(KColorScheme::ActiveBackground).color();
}


QColor K3b::Theme::foregroundColor() const
{
    if( m_fgColor.isValid() )
        return m_fgColor;
    else
        return KColorScheme(QPalette::Active, KColorScheme::Window).foreground(KColorScheme::ActiveText).color();
}


QPixmap K3b::Theme::pixmap( const QString& name ) const
{
    QMap<QString, QPixmap>::const_iterator it = m_pixmapMap.constFind( name );
    if( it != m_pixmapMap.constEnd() )
        return *it;

    // try loading the image
    if( QFile::exists( m_path + name ) ) {
        QPixmap pix( m_path + name );
        if ( !pix.isNull() )
            return *m_pixmapMap.insert( name, pix );
    }

    qDebug() << "(K3b::Theme)" << m_name << ": could not load image" << name << "in" << m_path;

    return m_emptyPixmap;
}


QPixmap K3b::Theme::pixmap( K3b::Theme::PixmapType t ) const
{
    return pixmap( filenameForPixmapType( t ) );
}


QPalette K3b::Theme::palette() const
{
    QPalette pal;
    pal.setColor( QPalette::Window, backgroundColor() );
    pal.setColor( QPalette::WindowText, foregroundColor() );
    return pal;
}


QString K3b::Theme::filenameForPixmapType( PixmapType t )
{
    QString name;

    switch( t ) {
    case MEDIA_AUDIO:
        name = "media_audio";
        break;
    case MEDIA_DATA:
        name = "media_data";
        break;
    case MEDIA_VIDEO:
        name = "media_video";
        break;
    case MEDIA_EMPTY:
        name = "media_empty";
        break;
    case MEDIA_MIXED:
        name = "media_mixed";
        break;
    case MEDIA_NONE:
        name = "media_none";
        break;
    case MEDIA_LEFT:
        name = "media_left";
        break;
    case PROGRESS_WORKING:
        name = "progress_working";
        break;
    case PROGRESS_SUCCESS:
        name = "progress_success";
        break;
    case PROGRESS_FAIL:
        name = "progress_fail";
        break;
    case PROGRESS_RIGHT:
        name = "progress_right";
        break;
    case DIALOG_LEFT:
        name = "dialog_left";
        break;
    case DIALOG_RIGHT:
        name = "dialog_right";
        break;
    case SPLASH:
        name = "splash";
        break;
    case PROJECT_LEFT:
        name = "project_left";
        break;
    case PROJECT_RIGHT:
        name = "project_right";
        break;
    case WELCOME_BG:
        name = "welcome_bg";
        break;
    default:
        break;
    }

    name.append( ".png" );

    return name;
}


K3b::Theme::BackgroundMode K3b::Theme::backgroundMode() const
{
    return m_bgMode;
}



class K3b::ThemeManager::Private
{
public:
    Private()
        : currentTheme(&emptyTheme) {
    }

    QList<K3b::Theme*> themes;
    QList<K3b::Theme*> gcThemes;
    K3b::Theme* currentTheme;
    QString currentThemeName;

    K3b::Theme emptyTheme;
};



K3b::ThemeManager::ThemeManager( QObject* parent )
    : QObject( parent )
{
    d = new Private();
    d->emptyTheme.m_name = "Empty Theme";
}


K3b::ThemeManager::~ThemeManager()
{
    for (QList<K3b::Theme*>::ConstIterator it = d->themes.constBegin(); it != d->themes.constEnd(); ++it)
        delete *it;
    qDeleteAll(d->gcThemes);
    d->themes.clear();
    delete d;
}


QList<K3b::Theme*>& K3b::ThemeManager::themes() const
{
    return d->themes;
}


K3b::Theme* K3b::ThemeManager::currentTheme() const
{
    return d->currentTheme;
}


void K3b::ThemeManager::readConfig( const KConfigGroup& c )
{
    // allow one to override the default theme by packaging a default config file
    QString defaultTheme = c.readEntry( "default theme", "quant" );

    K3b::Version configVersion( c.readEntry( "config version", "0.1" ) );
    if( configVersion >= K3b::Version("0.98") )
        setCurrentTheme( c.readEntry( "current theme", defaultTheme ) );
    else
        setCurrentTheme( defaultTheme );
}


void K3b::ThemeManager::saveConfig( KConfigGroup c )
{
    qDebug() << d->currentThemeName;
    if( !d->currentThemeName.isEmpty() ) {
        c.writeEntry( "current theme", d->currentThemeName );
    }
}


void K3b::ThemeManager::setCurrentTheme( const QString& name )
{
    if( name != d->currentThemeName ) {
        if( K3b::Theme* theme = findTheme( name ) )
            setCurrentTheme( theme );
    }
}


void K3b::ThemeManager::setCurrentTheme( K3b::Theme* theme )
{
    if( !theme && !d->themes.isEmpty() )
        theme = d->themes.first();

    if( theme ) {
        if( theme != d->currentTheme ) {
            d->currentTheme = theme;
            d->currentThemeName = theme->name();

            emit themeChanged();
            emit themeChanged( theme );
        }
    }
}


K3b::Theme* K3b::ThemeManager::findTheme( const QString& name ) const
{
    for( QList<K3b::Theme*>::ConstIterator it = d->themes.constBegin(); it != d->themes.constEnd(); ++it )
        if( (*it)->name() == name )
            return *it;
    return 0;
}


void K3b::ThemeManager::loadThemes()
{
    // first we cleanup the loaded themes
    for (QList<K3b::Theme*>::ConstIterator it = d->themes.constBegin(); it != d->themes.constEnd(); ++it)
        d->gcThemes << *it;
    d->themes.clear();

    QStringList dirs = QStandardPaths::locateAll( QStandardPaths::GenericDataLocation, "k3b/pics", QStandardPaths::LocateDirectory );
    // now search for themes. As there may be multiple themes with the same name
    // we only use the names from this list and then use findResourceDir to make sure
    // the local is preferred over the global stuff (like testing a theme by copying it
    // to the .kde dir)
    QStringList themeNames;
    for( QStringList::const_iterator dirIt = dirs.constBegin(); dirIt != dirs.constEnd(); ++dirIt ) {
        QDir dir( *dirIt );
        QStringList entries = dir.entryList( QDir::Dirs|QDir::NoDotAndDotDot );
        // every theme dir needs to contain a k3b.theme file
        for( QStringList::const_iterator entryIt = entries.constBegin(); entryIt != entries.constEnd(); ++entryIt ) {
            QString themeDir = *dirIt + '/' + *entryIt + '/';
            if( !themeNames.contains( *entryIt ) && QFile::exists( themeDir + "k3b.theme" ) ) {
                bool themeValid = true;

                // check for all nessessary pixmaps (this is a little evil hacking)
                for( int i = 0; i <= K3b::Theme::WELCOME_BG; ++i ) {
                    if( !QFile::exists( themeDir + K3b::Theme::filenameForPixmapType( (K3b::Theme::PixmapType)i ) ) ) {
                        qDebug() << "(K3b::ThemeManager) theme misses pixmap: " << K3b::Theme::filenameForPixmapType( (K3b::Theme::PixmapType)i );
                        themeValid = false;
                        break;
                    }
                }

                if( themeValid )
                    themeNames.append( *entryIt );
            }
        }
    }

    // now load the themes
    for( QStringList::const_iterator themeIt = themeNames.constBegin(); themeIt != themeNames.constEnd(); ++themeIt )
        loadTheme( *themeIt );

    // load the current theme
    setCurrentTheme( findTheme(d->currentThemeName) );
}


void K3b::ThemeManager::loadTheme( const QString& name )
{
    K3b::Theme* t = new K3b::Theme( name );
    if( !t->m_path.isEmpty() ) {
        t->m_name = name;
        t->m_local = QFileInfo( t->m_path ).isWritable();

        // load the stuff
        KConfig cfg( t->m_path + "/k3b.theme" );
        KConfigGroup group(&cfg,"");
        t->m_author = group.readEntry( "Author" );
        t->m_comment = group.readEntry( "Comment" );
        t->m_version = group.readEntry( "Version" );
        t->m_bgColor = group.readEntry( "Backgroundcolor", QColor() );
        t->m_fgColor = group.readEntry( "Foregroundcolor", QColor() );
        t->m_bgMode = ( group.readEntry( "BackgroundMode" ) == "Scaled" ? K3b::Theme::BG_SCALE : K3b::Theme::BG_TILE );

        d->themes.append( t );
    } else
	delete t;
}
