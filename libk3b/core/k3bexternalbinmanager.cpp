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
        return bin1->version() > bin2->version();
    }

    const int EXECUTE_TIMEOUT = 5000; // in seconds
}


// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALBIN
//
// ///////////////////////////////////////////////////////////

class K3b::ExternalBin::Private
{
public:
    Private( ExternalProgram& pr, const QString& pa )
        : program( pr ), path( pa ) {}

    ExternalProgram& program;
    QString path;
    Version version;
    QString copyright;
    QStringList features;
};

K3b::ExternalBin::ExternalBin( ExternalProgram& program, const QString& path )
    : d( new Private( program, path ) )
{
}


K3b::ExternalBin::~ExternalBin()
{
    delete d;
}

void K3b::ExternalBin::setVersion( const Version& version )
{
    d->version = version;
}

const K3b::Version& K3b::ExternalBin::version() const
{
    return d->version;
}

void K3b::ExternalBin::setCopyright( const QString& copyright )
{
    d->copyright = copyright;
}

const QString& K3b::ExternalBin::copyright() const
{
    return d->copyright;
}


bool K3b::ExternalBin::isEmpty() const
{
    return !d->version.isValid();
}


const QString& K3b::ExternalBin::path() const
{
    return d->path;
}


QString K3b::ExternalBin::name() const
{
    return d->program.name();
}


bool K3b::ExternalBin::hasFeature( const QString& f ) const
{
    return d->features.contains( f );
}


void K3b::ExternalBin::addFeature( const QString& f )
{
    d->features.append( f );
}


QStringList K3b::ExternalBin::userParameters() const
{
    return d->program.userParameters();
}


QStringList K3b::ExternalBin::features() const
{
    return d->features;
}


K3b::ExternalProgram& K3b::ExternalBin::program() const
{
    return d->program;
}



// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALPROGRAM
//
// ///////////////////////////////////////////////////////////


class K3b::ExternalProgram::Private
{
public:
    Private( const QString& n )
        : name( n ) {}

    QString name;
    QStringList userParameters;
    QList<const ExternalBin*> bins;
    QString defaultBin;
};


K3b::ExternalProgram::ExternalProgram( const QString& name )
    : d( new Private( name ) )
{
}


K3b::ExternalProgram::~ExternalProgram()
{
    qDeleteAll( d->bins );
    delete d;
}


const K3b::ExternalBin* K3b::ExternalProgram::mostRecentBin() const
{
    if ( d->bins.isEmpty() ) {
        return 0;
    }
    else {
        return d->bins.first();
    }
}


const K3b::ExternalBin* K3b::ExternalProgram::defaultBin() const
{
    if( d->bins.size() == 1 ) {
        return d->bins.first();
    }
    else {
        for( QList<const K3b::ExternalBin*>::const_iterator it = d->bins.constBegin(); it != d->bins.constEnd(); ++it ) {
            if( ( *it )->path() == d->defaultBin ) {
                return *it;
            }
        }
        return 0;
    }
}


void K3b::ExternalProgram::addBin( K3b::ExternalBin* bin )
{
    if( !d->bins.contains( bin ) ) {
        d->bins.append( bin );

        // the first bin in the list is always the one used
        // so we default to using the newest one
        qSort( d->bins.begin(), d->bins.end(), compareVersions );

        const ExternalBin* defBin = defaultBin();
        if ( !defBin || bin->version() > defBin->version() ) {
            setDefault( bin );
        }
    }
}


void K3b::ExternalProgram::clear()
{
    d->bins.clear();
}


void K3b::ExternalProgram::setDefault( const K3b::ExternalBin* bin )
{
    for( QList<const K3b::ExternalBin*>::const_iterator it = d->bins.constBegin(); it != d->bins.constEnd(); ++it ) {
        if( *it == bin ) {
            d->defaultBin = (*it)->path();
            break;
        }
    }
}


void K3b::ExternalProgram::setDefault( const QString& path )
{
    d->defaultBin = path;
}


QList<const K3b::ExternalBin*> K3b::ExternalProgram::bins() const
{
    return d->bins;
}


bool K3b::ExternalProgram::supportsUserParameters() const
{
    return true;
}


void K3b::ExternalProgram::addUserParameter( const QString& p )
{
    if( !d->userParameters.contains( p ) )
        d->userParameters.append(p);
}


void K3b::ExternalProgram::setUserParameters( const QStringList& list )
{
    d->userParameters = list;
}


QStringList K3b::ExternalProgram::userParameters() const
{
    return d->userParameters;
}


QString K3b::ExternalProgram::name() const
{
    return d->name;
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
        K3b::ExternalBin* bin = new ExternalBin( *this, path );

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
    vp << bin.path() << "--version";
    if( vp.execute( EXECUTE_TIMEOUT ) < 0 )
        return false;

    QString s = QString::fromLocal8Bit( vp.readAll() );
    bin.setVersion( parseVersion( s, bin ) );
    bin.setCopyright( parseCopyright( s, bin ) );
    return bin.version().isValid();
}


bool K3b::SimpleExternalProgram::scanFeatures( ExternalBin& bin ) const
{
#ifndef Q_OS_WIN32
    // check if we run as root
    struct stat s;
    if( !::stat( QFile::encodeName(bin.path()), &s ) ) {
        if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
            bin.addFeature( "suidroot" );
    }
#endif

    // probe features
    KProcess fp;
    fp.setOutputChannelMode( KProcess::MergedChannels );
    fp << bin.path() << "--help";
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


QString K3b::SimpleExternalProgram::versionIdentifier( const ExternalBin& /*bin*/ ) const
{
    return name();
}


// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALBINMANAGER
//
// ///////////////////////////////////////////////////////////


class K3b::ExternalBinManager::Private
{
public:
    QMap<QString, ExternalProgram*> programs;
    QStringList searchPath;

    static QString noPath;  // used for binPath() to return const string

    QString gatheredOutput;
};


QString K3b::ExternalBinManager::Private::noPath = "";


K3b::ExternalBinManager::ExternalBinManager( QObject* parent )
    : QObject( parent ),
      d( new Private )
{
}


K3b::ExternalBinManager::~ExternalBinManager()
{
    clear();
    delete d;
}


bool K3b::ExternalBinManager::readConfig( const KConfigGroup& grp )
{
    loadDefaultSearchPath();

    if( grp.hasKey( "search path" ) ) {
        setSearchPath( grp.readPathEntry( QString( "search path" ), QStringList() ) );
    }

    search();

    Q_FOREACH( K3b::ExternalProgram* p, d->programs ) {
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
        if( newestBin && newestBin->version() > lastMax )
            p->setDefault( newestBin );
    }

    return true;
}


bool K3b::ExternalBinManager::saveConfig( KConfigGroup grp )
{
    grp.writePathEntry( "search path", d->searchPath );

    Q_FOREACH( K3b::ExternalProgram* p, d->programs ) {
        if( p->defaultBin() )
            grp.writeEntry( p->name() + " default", p->defaultBin()->path() );

        grp.writeEntry( p->name() + " user parameters", p->userParameters() );

        const K3b::ExternalBin* newestBin = p->mostRecentBin();
        if( newestBin )
            grp.writeEntry( p->name() + " last seen newest version", newestBin->version().toString() );
    }

    return true;
}


bool K3b::ExternalBinManager::foundBin( const QString& name )
{
    if( d->programs.constFind( name ) == d->programs.constEnd() )
        return false;
    else
        return (d->programs[name]->defaultBin() != 0);
}


QString K3b::ExternalBinManager::binPath( const QString& name )
{
    if( d->programs.constFind( name ) == d->programs.constEnd() )
        return Private::noPath;

    if( d->programs[name]->defaultBin() != 0 )
        return d->programs[name]->defaultBin()->path();
    else
        return Private::noPath;
}


const K3b::ExternalBin* K3b::ExternalBinManager::binObject( const QString& name )
{
    if( d->programs.constFind( name ) == d->programs.constEnd() )
        return 0;

    return d->programs[name]->defaultBin();
}


void K3b::ExternalBinManager::addProgram( K3b::ExternalProgram* p )
{
    d->programs.insert( p->name(), p );
}


void K3b::ExternalBinManager::clear()
{
    qDeleteAll( d->programs );
    d->programs.clear();
}


void K3b::ExternalBinManager::search()
{
    if( d->searchPath.isEmpty() )
        loadDefaultSearchPath();

    Q_FOREACH( K3b::ExternalProgram* program, d->programs ) {
        program->clear();
    }

    // do not search one path twice
    QStringList paths;
    const QStringList possiblePaths = d->searchPath + KStandardDirs::systemPaths();
    foreach( QString p, possiblePaths ) {
        if (p.length() == 0)
            continue;
        if( p[p.length()-1] == '/' )
            p.truncate( p.length()-1 );
        if( !paths.contains( p ) && !paths.contains( p + "/" ) )
            paths.append(p);
    }

    Q_FOREACH( const QString& path, paths ) {
        Q_FOREACH( K3b::ExternalProgram* program, d->programs ) {
            program->scan( path );
        }
    }
}


K3b::ExternalProgram* K3b::ExternalBinManager::program( const QString& name ) const
{
    if( d->programs.constFind( name ) == d->programs.constEnd() )
        return 0;
    else
        return d->programs[name];
}


QMap<QString, K3b::ExternalProgram*> K3b::ExternalBinManager::programs() const
{
    return d->programs;
}


void K3b::ExternalBinManager::loadDefaultSearchPath()
{
    static const char* defaultSearchPaths[] = {
#ifndef Q_OS_WIN32
                                                "/usr/bin/",
                                                "/usr/local/bin/",
                                                "/usr/sbin/",
                                                "/usr/local/sbin/",
                                                "/sbin",
#endif
                                                0 };

    d->searchPath.clear();
    for( int i = 0; defaultSearchPaths[i]; ++i ) {
        d->searchPath.append( defaultSearchPaths[i] );
    }
}


QStringList K3b::ExternalBinManager::searchPath() const
{
    return d->searchPath;
}


void K3b::ExternalBinManager::setSearchPath( const QStringList& list )
{
    d->searchPath.clear();
    for( QStringList::const_iterator it = list.constBegin(); it != list.constEnd(); ++it ) {
        d->searchPath.append( QDir::fromNativeSeparators( *it ) );
    }
}


void K3b::ExternalBinManager::addSearchPath( const QString& path )
{
    QString aPath = QDir::fromNativeSeparators( path );
    if( !d->searchPath.contains( aPath ) )
        d->searchPath.append( aPath );
}


const K3b::ExternalBin* K3b::ExternalBinManager::mostRecentBinObject( const QString& name )
{
    if( K3b::ExternalProgram* p = program( name ) )
        return p->mostRecentBin();
    else
        return 0;
}

#include "k3bexternalbinmanager.moc"

