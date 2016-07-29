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


#include "k3bmovixprogram.h"

#include <kprocess.h>
#include <kdebug.h>
#include <klocale.h>

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>


K3b::MovixProgram::MovixProgram()
    : K3b::ExternalProgram( "eMovix" )
{
}


bool K3b::MovixProgram::scan( const QString& p )
{
    if( p.isEmpty() )
        return false;

    QString path = p;
    if( path[path.length()-1] != '/' )
        path.append("/");

    // first test if we have a version info (eMovix >= 0.8.0pre3)
    if( !QFile::exists( path + "movix-version" ) )
        return false;

    K3b::MovixBin* bin = 0;

    //
    // probe version and data dir
    //
    KProcess vp, dp;
    vp << path + "movix-version";
    dp << path + "movix-conf";
    vp.setOutputChannelMode( KProcess::MergedChannels );
    dp.setOutputChannelMode( KProcess::MergedChannels );
    vp.start();
    dp.start();
    if( vp.waitForFinished( -1 ) && dp.waitForFinished( -1 ) ) {
        QByteArray vout = vp.readAll();
        QByteArray dout = dp.readAll();
        // movix-version just gives us the version number on stdout
        if( !vout.isEmpty() && !dout.isEmpty() ) {
            bin = new K3b::MovixBin( *this, path );
            bin->setVersion( Version( vout.trimmed() ) );
            bin->m_movixPath = dout.trimmed();
        }
    }
    else {
        kDebug() << "(K3b::MovixProgram) could not start " << path << "movix-version";
        return false;
    }

    if( bin->version() >= K3b::Version( 0, 9, 0 ) )
        return scanNewEMovix( bin, path );
    else
        return scanOldEMovix( bin, path );
}


bool K3b::MovixProgram::scanNewEMovix( K3b::MovixBin* bin, const QString& path )
{
    QStringList files = bin->files();
    for( QStringList::iterator it = files.begin();
         it != files.end(); ++it ) {
        if( (*it).contains( "isolinux.cfg" ) ) {
            bin->m_supportedBootLabels = determineSupportedBootLabels( QString(*it).split( ' ' )[1] );
            break;
        }
    }

    // here we simply check for the movix-conf program
    if( QFile::exists( path + "movix-conf" ) ) {
        bin->addFeature( "newfiles" );
        addBin(bin);
        return true;
    }
    else {
        delete bin;
        return false;
    }
}


bool K3b::MovixProgram::scanOldEMovix( K3b::MovixBin* bin, const QString& path )
{
    //
    // first check if all necessary directories are present
    //
    QDir dir( bin->movixDataDir() );
    QStringList subdirs = dir.entryList( QDir::Dirs );
    if( !subdirs.contains( "boot-messages" ) ) {
        kDebug() << "(K3b::MovixProgram) could not find subdir 'boot-messages'";
        delete bin;
        return false;
    }
    if( !subdirs.contains( "isolinux" ) ) {
        kDebug() << "(K3b::MovixProgram) could not find subdir 'isolinux'";
        delete bin;
        return false;
    }
    if( !subdirs.contains( "movix" ) ) {
        kDebug() << "(K3b::MovixProgram) could not find subdir 'movix'";
        delete bin;
        return false;
    }
    if( !subdirs.contains( "mplayer-fonts" ) ) {
        kDebug() << "(K3b::MovixProgram) could not find subdir 'mplayer-fonts'";
        delete bin;
        return false;
    }


    //
    // check if we have a version of eMovix which contains the movix-files script
    //
    if( QFile::exists( path + "movix-files" ) ) {
        bin->addFeature( "files" );

        KProcess p;
        p << bin->path() + "movix-files";
        p.setOutputChannelMode( KProcess::MergedChannels );
        p.start();
        if( p.waitForFinished( -1 ) ) {
            bin->m_movixFiles = QString(p.readAll()).split( '\n' );
        }
    }

    //
    // fallback: to be compatible with 0.8.0rc2 we just add all files in the movix directory
    //
    if( bin->m_movixFiles.isEmpty() ) {
        QDir dir( bin->movixDataDir() + "/movix" );
        bin->m_movixFiles = dir.entryList(QDir::Files);
    }

    //
    // these files are fixed. That should not be a problem
    // since Isolinux is quite stable as far as I know.
    //
    bin->m_isolinuxFiles.append( "initrd.gz" );
    bin->m_isolinuxFiles.append( "isolinux.bin" );
    bin->m_isolinuxFiles.append( "isolinux.cfg" );
    bin->m_isolinuxFiles.append( "kernel/vmlinuz" );
    bin->m_isolinuxFiles.append( "movix.lss" );
    bin->m_isolinuxFiles.append( "movix.msg" );


    //
    // check every single necessary file :(
    //
    for( QStringList::const_iterator it = bin->m_isolinuxFiles.constBegin();
         it != bin->m_isolinuxFiles.constEnd(); ++it ) {
        if( !QFile::exists( bin->movixDataDir() + "/isolinux/" + *it ) ) {
            kDebug() << "(K3b::MovixProgram) Could not find file " << *it;
            delete bin;
            return false;
        }
    }

    //
    // now check the boot-messages languages
    //
    dir.cd( "boot-messages" );
    bin->m_supportedLanguages = dir.entryList( QDir::Dirs|QDir::NoDotAndDotDot );
    bin->m_supportedLanguages.removeAll("CVS");  // the eMovix makefile stuff seems not perfect ;)
    bin->m_supportedLanguages.prepend( i18n("default") );
    dir.cdUp();

    //
    // now check the supported mplayer-fontsets
    // FIXME: every font dir needs to contain the "font.desc" file!
    //
    dir.cd( "mplayer-fonts" );
    bin->m_supportedSubtitleFonts = dir.entryList( QDir::Dirs|QDir::NoDotAndDotDot );
    bin->m_supportedSubtitleFonts.removeAll("CVS");  // the eMovix makefile stuff seems not perfect ;)
    // new ttf fonts in 0.8.0rc2
    bin->m_supportedSubtitleFonts += dir.entryList( QStringList() << "*.ttf", QDir::Files );
    bin->m_supportedSubtitleFonts.prepend( i18n("none") );
    dir.cdUp();

    //
    // now check the supported boot labels
    //
    dir.cd( "isolinux" );
    bin->m_supportedBootLabels = determineSupportedBootLabels( dir.filePath("isolinux.cfg") );

    //
    // This seems to be a valid eMovix installation. :)
    //

    addBin(bin);
    return true;
}


QStringList K3b::MovixProgram::determineSupportedBootLabels( const QString& isoConfigFile ) const
{
    QStringList list( i18n("default") );

    QFile f( isoConfigFile );
    if( !f.open( QIODevice::ReadOnly ) ) {
        kDebug() << "(K3b::MovixProgram) could not open file '" << f.fileName() << "'";
    }
    else {
        QTextStream fs( &f );
        QString line = fs.readLine();
        while( !line.isNull() ) {
            if( line.startsWith( "label" ) )
                list.append( line.mid( 5 ).trimmed() );

            line = fs.readLine();
        }
        f.close();
    }

    return list;
}


QString K3b::MovixBin::subtitleFontDir( const QString& font ) const
{
    if( font == i18n("none" ) )
        return "";
    else if( m_supportedSubtitleFonts.contains( font ) )
        return path() + "/mplayer-fonts/" + font;
    else
        return "";
}


QString K3b::MovixBin::languageDir( const QString& lang ) const
{
    if( lang == i18n("default") )
        return languageDir( "en" );
    else if( m_supportedLanguages.contains( lang ) )
        return path() + "/boot-messages/" + lang;
    else
        return "";
}


QStringList K3b::MovixBin::supportedSubtitleFonts() const
{
    if( version() >= K3b::Version( 0, 9, 0 ) )
        return QStringList( i18n("default") ) += supported( "font" );
    else
        return m_supportedSubtitleFonts;
}


QStringList K3b::MovixBin::supportedLanguages() const
{
    if( version() >= K3b::Version( 0, 9, 0 ) )
        return QStringList( i18n("default") ) += supported( "lang" );
    else
        return m_supportedLanguages;
}


// only used for eMovix >= 0.9.0
QStringList K3b::MovixBin::supportedKbdLayouts() const
{
    return QStringList( i18n("default") ) += supported( "kbd" );
}


// only used for eMovix >= 0.9.0
QStringList K3b::MovixBin::supportedBackgrounds() const
{
    return QStringList( i18n("default") ) += supported( "background" );
}


// only used for eMovix >= 0.9.0
QStringList K3b::MovixBin::supportedCodecs() const
{
    return supported( "codecs" );
}


QStringList K3b::MovixBin::supported( const QString& type ) const
{
    KProcess p;
    p << path() + "movix-conf" << "--supported=" + type;
    p.setOutputChannelMode( KProcess::MergedChannels );
    p.start();
    if( p.waitForFinished( -1 ) )
        return QString(p.readAll()).split( '\n', QString::SkipEmptyParts );
    else
        return QStringList();
}


QStringList K3b::MovixBin::files( const QString& kbd,
                                  const QString& font,
                                  const QString& bg,
                                  const QString& lang,
                                  const QStringList& codecs ) const
{
    KProcess p;
    p << path() + "movix-conf" << "--files";
    p.setOutputChannelMode( KProcess::MergedChannels );

    if( !kbd.isEmpty() && kbd != i18n("default") )
        p << "--kbd" << kbd;
    if( !font.isEmpty() && font != i18n("default") )
        p << "--font" << font;
    if( !bg.isEmpty() && bg != i18n("default") )
        p << "--background" << bg;
    if( !lang.isEmpty() && lang != i18n("default") )
        p << "--lang" << lang;
    if( !codecs.isEmpty() )
        p << "--codecs" << codecs.join( "," );

    p.start();
    if( p.waitForFinished( -1 ) )
        return QString(p.readAll()).split( '\n', QString::SkipEmptyParts );
    else
        return QStringList();
}
