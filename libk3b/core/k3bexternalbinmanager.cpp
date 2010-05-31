/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bexternalbinmanager.h"
#include "k3bglobals.h"

#include <KConfigGroup>
#include <KDebug>
#include <kdeversion.h>
#include <kde_file.h>
#include <KProcess>
#include <KStandardDirs>

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QRegExp>

#ifndef Q_OS_WIN32
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#endif


namespace {
    bool compareVersions( const K3b::ExternalBin* bin1, const K3b::ExternalBin* bin2 )
    {
        return bin1->version > bin2->version;
    }
    
    const int EXECUTE_TIMEOUT = 5000; // in seconds
}


QString K3b::ExternalBinManager::m_noPath = "";


// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALBIN
//
// ///////////////////////////////////////////////////////////

K3b::ExternalBin::ExternalBin( K3b::ExternalProgram* p )
    : m_program(p)
{
}


bool K3b::ExternalBin::isEmpty() const
{
    return !version.isValid();
}


QString K3b::ExternalBin::name() const
{
    return m_program->name();
}


bool K3b::ExternalBin::hasFeature( const QString& f ) const
{
    return m_features.contains( f );
}


void K3b::ExternalBin::addFeature( const QString& f )
{
    m_features.append( f );
}


QStringList K3b::ExternalBin::userParameters() const
{
    return m_program->userParameters();
}


QStringList K3b::ExternalBin::features() const
{
    return m_features;
}


K3b::ExternalProgram* K3b::ExternalBin::program() const
{
    return m_program;
}



// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALPROGRAM
//
// ///////////////////////////////////////////////////////////


K3b::ExternalProgram::ExternalProgram( const QString& name )
    : m_name( name ),
      m_defaultBin( 0 )
{
}


K3b::ExternalProgram::~ExternalProgram()
{
    qDeleteAll( m_bins );
}


const K3b::ExternalBin* K3b::ExternalProgram::mostRecentBin() const
{
    if ( m_bins.isEmpty() ) {
        return 0;
    }
    else {
        return m_bins.first();
    }
}


const K3b::ExternalBin* K3b::ExternalProgram::defaultBin() const
{
    if( m_bins.size() == 1 ) {
        return m_bins.first();
    }
    else {
        for( QList<const K3b::ExternalBin*>::const_iterator it = m_bins.constBegin(); it != m_bins.constEnd(); ++it ) {
            if( ( *it )->path == m_defaultBin ) {
                return *it;
            }
        }
        return 0;
    }
}


void K3b::ExternalProgram::addBin( K3b::ExternalBin* bin )
{
    if( !m_bins.contains( bin ) ) {
        m_bins.append( bin );

        // the first bin in the list is always the one used
        // so we default to using the newest one
        qSort( m_bins.begin(), m_bins.end(), compareVersions );

        const ExternalBin* defBin = defaultBin();
        if ( !defBin || bin->version > defBin->version ) {
            setDefault( bin );
        }
    }
}


void K3b::ExternalProgram::setDefault( const K3b::ExternalBin* bin )
{
    for( QList<const K3b::ExternalBin*>::const_iterator it = m_bins.constBegin(); it != m_bins.constEnd(); ++it ) {
        if( *it == bin ) {
            m_defaultBin = (*it)->path;
            break;
        }
    }
}


void K3b::ExternalProgram::setDefault( const QString& path )
{
    m_defaultBin = path;
}


void K3b::ExternalProgram::addUserParameter( const QString& p )
{
    if( !m_userParameters.contains( p ) )
        m_userParameters.append(p);
}


// static
QString K3b::ExternalProgram::buildProgramPath( const QString& dir, const QString& programName )
{
    QString p = K3b::prepareDir( dir ) + programName;
#ifdef Q_OS_WIN32
    p += ".exe";
#endif
    return p;
}


// ///////////////////////////////////////////////////////////
//
// SIMPLEEXTERNALPROGRAM
//
// ///////////////////////////////////////////////////////////

class K3b::SimpleExternalProgram::Private
{
public:
};


K3b::SimpleExternalProgram::SimpleExternalProgram( const QString& name )
    : ExternalProgram( name ),
      d( new Private() )
{
}


K3b::SimpleExternalProgram::~SimpleExternalProgram()
{
    delete d;
}


QString K3b::SimpleExternalProgram::getProgramPath( const QString& dir ) const
{
    return buildProgramPath( dir, name() );
}


bool K3b::SimpleExternalProgram::scan( const QString& p )
{
    if( p.isEmpty() )
        return false;

    QString path = getProgramPath( p );

    if ( QFile::exists( path ) ) {
        K3b::ExternalBin* bin = new ExternalBin( this );
        bin->path = path;

        if ( !scanVersion( *bin ) ||
             !scanFeatures( *bin ) ) {
            delete bin;
            return false;
        }

        addBin( bin );
        return true;
    }
    else {
        return false;
    }
}


bool K3b::SimpleExternalProgram::scanVersion( ExternalBin& bin ) const
{
    // probe version
    KProcess vp;
    vp.setOutputChannelMode( KProcess::MergedChannels );
    vp << bin.path << "--version";
    if( vp.execute( EXECUTE_TIMEOUT ) < 0 )
        return false;

    QString s = QString::fromLocal8Bit( vp.readAll() );
    bin.version = parseVersion( s, bin );
    bin.copyright = parseCopyright( s, bin );
    return bin.version.isValid();
}


bool K3b::SimpleExternalProgram::scanFeatures( ExternalBin& bin ) const
{
#ifndef Q_OS_WIN32
    // check if we run as root
    struct stat s;
    if( !::stat( QFile::encodeName(bin.path), &s ) ) {
        if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
            bin.addFeature( "suidroot" );
    }
#endif

    // probe features
    KProcess fp;
    fp.setOutputChannelMode( KProcess::MergedChannels );
    fp << bin.path << "--help";
    if( fp.execute( EXECUTE_TIMEOUT ) < 0 )
        return false;

    parseFeatures( QString::fromLocal8Bit( fp.readAll() ), bin );
    return true;
}


K3b::Version K3b::SimpleExternalProgram::parseVersion( const QString& out, const ExternalBin& bin ) const
{
    // we first look for the program name with first upper char so we do not catch
    // the warning messages on stderr (cdrecord sometimes produces those)
    QString programName = versionIdentifier( bin );
    QString programNameCap = programName[0].toUpper() + programName.mid( 1 );
    int pos = out.indexOf( programNameCap );
    if ( pos < 0 )
        pos = out.indexOf( programName );

    if( pos < 0 )
        return Version();

    return parseVersionAt( out, pos );
}


QString K3b::SimpleExternalProgram::parseCopyright( const QString& output, const ExternalBin& /*bin*/ ) const
{
    int pos = output.indexOf( "(C)", 0 );
    if ( pos < 0 )
        return QString();
    pos += 4;
    int endPos = output.indexOf( '\n', pos );
    return output.mid( pos, endPos-pos );
}


void K3b::SimpleExternalProgram::parseFeatures( const QString& /*output*/, ExternalBin& /*bin*/ ) const
{
    // do nothing
}


// static
K3b::Version K3b::SimpleExternalProgram::parseVersionAt( const QString& data, int pos )
{
    int sPos = data.indexOf( QRegExp("\\d"), pos );
    if( sPos < 0 )
        return Version();

    int endPos = data.indexOf( QRegExp("[\\s,]"), sPos + 1 );
    if( endPos < 0 )
        return Version();

    return data.mid( sPos, endPos - sPos );
}


QString K3b::SimpleExternalProgram::versionIdentifier( const ExternalBin& bin ) const
{
    return name();
}


// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALBINMANAGER
//
// ///////////////////////////////////////////////////////////


K3b::ExternalBinManager::ExternalBinManager( QObject* parent )
    : QObject( parent )
{
}


K3b::ExternalBinManager::~ExternalBinManager()
{
    clear();
}


bool K3b::ExternalBinManager::readConfig( const KConfigGroup& grp )
{
    loadDefaultSearchPath();

    if( grp.hasKey( "search path" ) ) {
        setSearchPath( grp.readPathEntry( QString( "search path" ), QStringList() ) );
    }

    search();

    Q_FOREACH( K3b::ExternalProgram* p, m_programs ) {
        if( grp.hasKey( p->name() + " default" ) ) {
            p->setDefault( grp.readEntry( p->name() + " default", QString() ) );
        }

        QStringList list = grp.readEntry( p->name() + " user parameters", QStringList() );
        for( QStringList::const_iterator strIt = list.constBegin(); strIt != list.constEnd(); ++strIt )
            p->addUserParameter( *strIt );

        K3b::Version lastMax( grp.readEntry( p->name() + " last seen newest version", QString() ) );
        // now search for a newer version and use it (because it was installed after the last
        // K3b run and most users would probably expect K3b to use a newly installed version)
        const K3b::ExternalBin* newestBin = p->mostRecentBin();
        if( newestBin && newestBin->version > lastMax )
            p->setDefault( newestBin );
    }

    return true;
}


bool K3b::ExternalBinManager::saveConfig( KConfigGroup grp )
{
    grp.writePathEntry( "search path", m_searchPath );

    Q_FOREACH( K3b::ExternalProgram* p, m_programs ) {
        if( p->defaultBin() )
            grp.writeEntry( p->name() + " default", p->defaultBin()->path );

        grp.writeEntry( p->name() + " user parameters", p->userParameters() );

        const K3b::ExternalBin* newestBin = p->mostRecentBin();
        if( newestBin )
            grp.writeEntry( p->name() + " last seen newest version", newestBin->version.toString() );
    }

    return true;
}


bool K3b::ExternalBinManager::foundBin( const QString& name )
{
    if( m_programs.constFind( name ) == m_programs.constEnd() )
        return false;
    else
        return (m_programs[name]->defaultBin() != 0);
}


QString K3b::ExternalBinManager::binPath( const QString& name )
{
    if( m_programs.constFind( name ) == m_programs.constEnd() )
        return m_noPath;

    if( m_programs[name]->defaultBin() != 0 )
        return m_programs[name]->defaultBin()->path;
    else
        return m_noPath;
}


const K3b::ExternalBin* K3b::ExternalBinManager::binObject( const QString& name )
{
    if( m_programs.constFind( name ) == m_programs.constEnd() )
        return 0;

    return m_programs[name]->defaultBin();
}


void K3b::ExternalBinManager::addProgram( K3b::ExternalProgram* p )
{
    m_programs.insert( p->name(), p );
}


void K3b::ExternalBinManager::clear()
{
    qDeleteAll( m_programs );
    m_programs.clear();
}


void K3b::ExternalBinManager::search()
{
    if( m_searchPath.isEmpty() )
        loadDefaultSearchPath();

    Q_FOREACH( K3b::ExternalProgram* program, m_programs ) {
        program->clear();
    }

    // do not search one path twice
    QStringList paths;
    const QStringList possiblePaths = m_searchPath + KStandardDirs::systemPaths();
    foreach( QString p, possiblePaths ) {
        if (p.length() == 0)
            continue;
        if( p[p.length()-1] == '/' )
            p.truncate( p.length()-1 );
        if( !paths.contains( p ) && !paths.contains( p + "/" ) )
            paths.append(p);
    }

    Q_FOREACH( const QString& path, paths ) {
        Q_FOREACH( K3b::ExternalProgram* program, m_programs ) {
            program->scan( path );
        }
    }
}


K3b::ExternalProgram* K3b::ExternalBinManager::program( const QString& name ) const
{
    if( m_programs.find( name ) == m_programs.constEnd() )
        return 0;
    else
        return m_programs[name];
}


void K3b::ExternalBinManager::loadDefaultSearchPath()
{
    static const char* defaultSearchPaths[] = {
#ifndef Q_OS_WIN32
                                                "/usr/bin/",
                                                "/usr/local/bin/",
                                                "/usr/sbin/",
                                                "/usr/local/sbin/",
                                                "/opt/schily/bin/",
                                                "/sbin",
#endif
                                                0 };

    m_searchPath.clear();
    for( int i = 0; defaultSearchPaths[i]; ++i ) {
        m_searchPath.append( defaultSearchPaths[i] );
    }
}


void K3b::ExternalBinManager::setSearchPath( const QStringList& list )
{
    loadDefaultSearchPath();

    for( QStringList::const_iterator it = list.constBegin(); it != list.constEnd(); ++it ) {
        QString aPath = QDir::fromNativeSeparators( *it );
        if( !m_searchPath.contains( aPath ) )
            m_searchPath.append( aPath );
    }
}


void K3b::ExternalBinManager::addSearchPath( const QString& path )
{
    QString aPath = QDir::fromNativeSeparators( path );
    if( !m_searchPath.contains( aPath ) )
        m_searchPath.append( aPath );
}



const K3b::ExternalBin* K3b::ExternalBinManager::mostRecentBinObject( const QString& name )
{
    if( K3b::ExternalProgram* p = program( name ) )
        return p->mostRecentBin();
    else
        return 0;
}

#include "k3bexternalbinmanager.moc"

