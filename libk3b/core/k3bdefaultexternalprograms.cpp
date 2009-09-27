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


#include "k3bdefaultexternalprograms.h"
#include "k3bexternalbinmanager.h"
#include "k3bglobals.h"
#include "k3bprocess.h"

#include <qfile.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qobject.h>
#include <qregexp.h>
#include <qtextstream.h>

#include <kdebug.h>
#include <KProcess>


void K3b::addDefaultPrograms( K3b::ExternalBinManager* m )
{
    m->addProgram( new K3b::CdrecordProgram() );
    m->addProgram( new K3b::MkisofsProgram() );
    m->addProgram( new K3b::ReadcdProgram() );
    m->addProgram( new K3b::CdrdaoProgram() );
    m->addProgram( new K3b::GrowisofsProgram() );
    m->addProgram( new K3b::DvdformatProgram() );
    //  m->addProgram( new K3b::DvdBooktypeProgram() );
}


void K3b::addTranscodePrograms( K3b::ExternalBinManager* m )
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
        m->addProgram( new K3b::TranscodeProgram( transcodeTools[i] ) );
}


void K3b::addVcdimagerPrograms( K3b::ExternalBinManager* m )
{
    // don't know if we need more vcdTools in the future (vcdxrip)
    static const char* vcdTools[] =  { "vcdxbuild",
                                       "vcdxminfo",
                                       "vcdxrip",
                                       0 };

    for( int i = 0; vcdTools[i]; ++i )
        m->addProgram( new K3b::VcdbuilderProgram( vcdTools[i] ) );
}


K3b::AbstractCdrtoolsProgram::AbstractCdrtoolsProgram( const QString& program, const QString& cdrkitAlternative )
    : SimpleExternalProgram( program ),
      m_cdrkitAlt( cdrkitAlternative )
{
}



#ifndef Q_OS_WIN32
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
        kDebug() << "checking for Debian cdrtools wrapper script.";
        if( QFileInfo( path ).size() < 1024 ) {
            kDebug() << "Debian Wrapper script size fits. Checking file.";
            QFile f( path );
            f.open( QIODevice::ReadOnly );
            QString s = QTextStream( &f ).readAll();
            if( s.contains( "cdrecord.mmap" ) && s.contains( "cdrecord.shm" ) ) {
                kDebug() << "Found Debian Wrapper script.";
                QString ext;
                if( K3b::kernelVersion().versionString().left(3) > "2.2" )
                    ext = ".mmap";
                else
                    ext = ".shm";

                kDebug() << "Using crtools" << ext;

                path += ext;
            }
        }
    }

    return path;
}
#endif


QString K3b::AbstractCdrtoolsProgram::getProgramPath( const QString& dir )
{
    QString cdrtoolsPath = ExternalProgram::buildProgramPath( dir, name() );
    QString cdrkitPath = ExternalProgram::buildProgramPath( dir, m_cdrkitAlt );

    QString path;
    if( QFile::exists( cdrtoolsPath ) &&
        QFileInfo(K3b::resolveLink( cdrtoolsPath )).baseName() != m_cdrkitAlt ) {
        m_usingCdrkit = false;
        path = cdrtoolsPath;
    }
    else if( QFile::exists( cdrkitPath ) ) {
        m_usingCdrkit = true;
        path = cdrkitPath;
    }

#ifndef Q_OS_WIN32
    if ( !path.isEmpty() && name() == QLatin1String( "cdrecord" ) ) {
        debianWeirdnessHack( path );
    }
#endif

    if ( m_usingCdrkit )
        setVersionIdentifier( m_cdrkitAlt );
#ifdef Q_OS_WIN32        
    else 
       setVersionIdentifier( name() );
#endif

    return path;
}


K3b::CdrecordProgram::CdrecordProgram()
    : K3b::AbstractCdrtoolsProgram( QLatin1String( "cdrecord" ), QLatin1String( "wodim" ) )
{
}


void K3b::CdrecordProgram::parseFeatures( const QString& out, ExternalBin* bin )
{
    if( m_usingCdrkit )
        bin->addFeature( "wodim" );

    if( bin->version.suffix().endsWith( "-dvd" ) ) {
        bin->addFeature( "dvd-patch" );
        bin->version = QString(bin->version.versionString()).remove("-dvd");
    }

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
        ( m_usingCdrkit || bin->version > K3b::Version( 2, 1, -1, "a14") ) ) // cuefile handling was still buggy in a14
        bin->addFeature( "cuefile" );

    // new mode 2 options since cdrecord 2.01a12
    // we use both checks here since the help was not updated in 2.01a12 yet (well, I
    // just double-checked and the help page is proper but there is no harm in having
    // two checks)
    // and the version check does not handle versions like 2.01-dvd properly
    if( out.contains( "-xamix" ) ||
        bin->version >= K3b::Version( 2, 1, -1, "a12" ) ||
        m_usingCdrkit )
        bin->addFeature( "xamix" );

    if( bin->version < K3b::Version( 2, 0 ) && !m_usingCdrkit )
        bin->addFeature( "outdated" );

    // FIXME: are these version correct?
    if( bin->version >= K3b::Version("1.11a38") || m_usingCdrkit )
        bin->addFeature( "plain-atapi" );
    if( bin->version > K3b::Version("1.11a17") || m_usingCdrkit )
        bin->addFeature( "hacked-atapi" );

    if( bin->version >= K3b::Version( 2, 1, 1, "a02" ) || m_usingCdrkit )
        bin->addFeature( "short-track-raw" );

    if( bin->version >= K3b::Version( 2, 1, -1, "a13" ) || m_usingCdrkit )
        bin->addFeature( "audio-stdin" );

    if( bin->version >= K3b::Version( "1.11a02" ) || m_usingCdrkit )
        bin->addFeature( "burnfree" );
    else
        bin->addFeature( "burnproof" );

    if ( bin->version >= K3b::Version( 2, 1, 1, "a29" ) && !m_usingCdrkit )
        bin->addFeature( "blu-ray" );

    // FIXME: when did cdrecord introduce free dvd support?
    bin->addFeature( "dvd" );
}



K3b::MkisofsProgram::MkisofsProgram()
    : K3b::AbstractCdrtoolsProgram( QLatin1String( "mkisofs" ), QLatin1String( "genisoimage" ) )
{
}

void K3b::MkisofsProgram::parseFeatures( const QString& out, ExternalBin* bin )
{
    if( m_usingCdrkit )
        bin->addFeature( "genisoimage" );

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

    if( bin->version < K3b::Version( 1, 14) && !m_usingCdrkit )
        bin->addFeature( "outdated" );

    if( bin->version >= K3b::Version( 1, 15, -1, "a40" ) || m_usingCdrkit )
        bin->addFeature( "backslashed_filenames" );

    if ( m_usingCdrkit && bin->version >= K3b::Version( 1, 1, 4 ) )
        bin->addFeature( "no-4gb-limit" );

    if ( !m_usingCdrkit && bin->version >= K3b::Version( 2, 1, 1, "a32" ) )
        bin->addFeature( "no-4gb-limit" );
}


K3b::ReadcdProgram::ReadcdProgram()
    : K3b::AbstractCdrtoolsProgram( QLatin1String( "readcd" ), QLatin1String( "readom" ) )
{
}

void K3b::ReadcdProgram::parseFeatures( const QString& out, ExternalBin* bin )
{
    if( m_usingCdrkit )
        bin->addFeature( "readom" );

    if( out.contains( "-clone" ) )
        bin->addFeature( "clone" );

    // FIXME: are these version correct?
    if( bin->version >= K3b::Version("1.11a38") || m_usingCdrkit )
        bin->addFeature( "plain-atapi" );
    if( bin->version > K3b::Version("1.11a17") || m_usingCdrkit )
        bin->addFeature( "hacked-atapi" );
}


K3b::Cdda2wavProgram::Cdda2wavProgram()
    : K3b::AbstractCdrtoolsProgram( QLatin1String( "cdda2wav" ), QLatin1String( "icedax" ) )
{
}

void K3b::Cdda2wavProgram::parseFeatures( const QString& out, ExternalBin* bin )
{
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


K3b::CdrdaoProgram::CdrdaoProgram()
    : K3b::SimpleExternalProgram( "cdrdao" )
{
    setVersionIdentifier( QLatin1String( "Cdrdao version" ) );
}


bool K3b::CdrdaoProgram::scanFeatures( ExternalBin* bin )
{
    // probe features
    KProcess fp;
    fp.setOutputChannelMode( KProcess::MergedChannels );
    fp << bin->path << "write" << "-h";

    if( fp.execute() >= 0 ) {
        QByteArray out = fp.readAll();
        if( out.contains( "--overburn" ) )
            bin->addFeature( "overburn" );
        if( out.contains( "--multi" ) )
            bin->addFeature( "multisession" );

        if( out.contains( "--buffer-under-run-protection" ) )
            bin->addFeature( "disable-burnproof" );

        // SuSE 9.0 ships with a patched cdrdao 1.1.7 which contains an updated libschily
        // Gentoo ships with a patched cdrdao 1.1.7 which contains scglib support
        if( bin->version > K3b::Version( 1, 1, 7 ) ||
            bin->version == K3b::Version( 1, 1, 7, "-gentoo" ) ||
            bin->version == K3b::Version( 1, 1, 7, "-suse" ) ) {
            //    bin->addFeature( "plain-atapi" );
            bin->addFeature( "hacked-atapi" );
        }

        if( bin->version >= K3b::Version( 1, 1, 8 ) )
            bin->addFeature( "plain-atapi" );

        return SimpleExternalProgram::scanFeatures( bin );
    }
    else {
        kDebug() << "could not start " << bin->path;
        return false;
    }
}


K3b::TranscodeProgram::TranscodeProgram( const QString& transcodeProgram )
    : K3b::SimpleExternalProgram( transcodeProgram )
{
    setVersionIdentifier( QLatin1String( "transcode v" ) );
}

bool K3b::TranscodeProgram::scanFeatures( ExternalBin* bin )
{
    //
    // Check features
    //
    QString modInfoBin = buildProgramPath( QFileInfo( bin->path ).absolutePath(), QLatin1String( "tcmodinfo" ) );
    Process modp;
    modp.setOutputChannelMode( KProcess::MergedChannels );
    modp << modInfoBin << "-p";

    if( !modp.execute() ) {
        QString modPath = QString::fromLocal8Bit( modp.readAll() ).simplified();
        QDir modDir( modPath );
        if( !modDir.entryList( QStringList() << "*export_xvid*", QDir::Files ).isEmpty() )
            bin->addFeature( "xvid" );
        if( !modDir.entryList( QStringList() << "*export_lame*", QDir::Files ).isEmpty() )
            bin->addFeature( "lame" );
        if( !modDir.entryList( QStringList() << "*export_ffmpeg*", QDir::Files ).isEmpty() )
            bin->addFeature( "ffmpeg" );
        if( !modDir.entryList( QStringList() << "*export_ac3*", QDir::Files ).isEmpty() )
            bin->addFeature( "ac3" );

        return true;
    }
    else {
        kDebug() << "Failed to start" << modp.program();
        return false;
    }
}


K3b::VcdbuilderProgram::VcdbuilderProgram( const QString& p )
    : K3b::SimpleExternalProgram( p )
{
    setVersionIdentifier( QLatin1String( "GNU VCDImager" ) );
}



K3b::NormalizeProgram::NormalizeProgram()
    : K3b::SimpleExternalProgram( "normalize" )
{
}


K3b::GrowisofsProgram::GrowisofsProgram()
    : K3b::SimpleExternalProgram( "growisofs" )
{
}

bool K3b::GrowisofsProgram::scanFeatures( ExternalBin* bin )
{
    // fixed Copyright:
    bin->copyright = "Andy Polyakov <appro@fy.chalmers.se>";

    if ( bin->version >= K3b::Version( 5, 20 ) )
        bin->addFeature( "dual-layer" );

    if ( bin->version > K3b::Version( 5, 17 ) )
        bin->addFeature( "tracksize" );

    if ( bin->version >= K3b::Version( 5, 15 ) )
        bin->addFeature( "daosize" );

    if ( bin->version >= K3b::Version( 6, 0 ) )
        bin->addFeature( "buffer" );

    if ( bin->version >= K3b::Version( 7, 0 ) )
        bin->addFeature( "blu-ray" );

    return SimpleExternalProgram::scanFeatures( bin );
}


K3b::DvdformatProgram::DvdformatProgram()
    : K3b::SimpleExternalProgram( "dvd+rw-format" )
{
}

K3b::Version K3b::DvdformatProgram::parseVersion( const QString& out )
{
    // different locales make searching for the +- char difficult
    // so we simply ignore it.
    int pos = out.indexOf( QRegExp("DVD.*RW(/-RAM)? format utility") );
    if( pos < 0 )
        return Version();

    pos = out.indexOf( "version", pos );
    if( pos < 0 )
        return Version();

    pos += 8;

    // the version ends in a dot.
    int endPos = out.indexOf( QRegExp("\\.\\D"), pos );
    if( endPos < 0 )
        return Version();

    return out.mid( pos, endPos-pos );
}

QString K3b::DvdformatProgram::parseCopyright( const QString& )
{
    // fixed Copyright:
    return QLatin1String( "Andy Polyakov <appro@fy.chalmers.se>" );
}


K3b::DvdBooktypeProgram::DvdBooktypeProgram()
    : K3b::SimpleExternalProgram( "dvd+rw-booktype" )
{
}

K3b::Version K3b::DvdBooktypeProgram::parseVersion( const QString& out )
{
    int pos = out.indexOf( "dvd+rw-booktype" );
    if( pos < 0 )
        return Version();

    // No version information. Create dummy version
    return K3b::Version( 1, 0, 0 );
}

QString K3b::DvdBooktypeProgram::parseCopyright( const QString& )
{
    // fixed Copyright:
    return QLatin1String( "Andy Polyakov <appro@fy.chalmers.se>" );
}
