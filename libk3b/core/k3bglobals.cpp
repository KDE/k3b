/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-k3b.h>

#include "k3bglobals.h"
#include "k3bglobalsettings.h"
#include "k3bversion.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bdeviceglobals.h"
#include "k3bexternalbinmanager.h"
#include "k3bcore.h"
#include "k3bmediacache.h"
#include "k3bmsf.h"
#include "k3b_i18n.h"

#include <KProcess>
#include <kio_version.h>
#include <KIO/Job>
#include <KIO/StatJob>
#include <KMountPoint>
#include <Solid/Device>
#include <Solid/StorageAccess>
#include <Solid/OpticalDrive>

#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUrl>
#include <QStorageInfo>

#include <cmath>
#include <sys/utsname.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#  include <sys/param.h>
#  include <sys/mount.h>
#  include <sys/endian.h>
#  define bswap_16(x) bswap16(x)
#  define bswap_32(x) bswap32(x)
#  define bswap_64(x) bswap64(x)
#else
#  include <byteswap.h>
#endif

#ifdef Q_OS_WIN32
#include <windows.h>
#endif


qint16 K3b::swapByteOrder( const qint16& i )
{
    return bswap_16( i );
    //((i << 8) & 0xff00) | ((i >> 8 ) & 0xff);
}


qint32 K3b::swapByteOrder( const qint32& i )
{
    //return ((i << 24) & 0xff000000) | ((i << 8) & 0xff0000) | ((i >> 8) & 0xff00) | ((i >> 24) & 0xff );
    return bswap_32( i );
}


qint64 K3b::swapByteOrder( const qint64& i )
{
    return bswap_64( i );
}


QString K3b::findUniqueFilePrefix( const QString& _prefix, const QString& path )
{
    QString url;
    if( path.isEmpty() || !QFile::exists(path) )
        url = defaultTempPath();
    else
        url = prepareDir( path );

    QString prefix = _prefix;
    if( prefix.isEmpty() )
        prefix = "k3b_";

    // now create the unique prefix
    QDir dir( url );
    QStringList entries = dir.entryList( QDir::NoFilter, QDir::Name );
    int i = 0;
    for( QStringList::iterator it = entries.begin();
         it != entries.end(); ++it ) {
        if( (*it).startsWith( prefix + QString::number(i) ) ) {
            i++;
            it = entries.begin();
        }
    }

    return url + prefix + QString::number(i);
}


QString K3b::findTempFile( const QString& ending, const QString& d )
{
    return findUniqueFilePrefix( "k3b_", d ) + ( ending.isEmpty() ? QString() : (QString::fromLatin1(".") + ending) );
}


QString K3b::defaultTempPath()
{
    return prepareDir( k3bcore->globalSettings()->defaultTempPath() );
}


QString K3b::prepareDir( const QString& dir )
{
    if(dir.isEmpty())
        return QString();
    else if ( !dir.endsWith( '/' ) )
        return dir + '/';
    else
        return dir;
}


QString K3b::parentDir( const QString& path )
{
    QString parent = path;
    if( path.isEmpty())
        return QString();
    if( path[path.length()-1] == '/' )
        parent.truncate( parent.length()-1 );

    int pos = parent.lastIndexOf( '/' );
    if( pos >= 0 )
        parent.truncate( pos+1 );
    else // relative path, do anything...
        parent = '/';

    return parent;
}


QString K3b::fixupPath( const QString& path )
{
    QString s;
    bool lastWasSlash = false;
    for( int i = 0; i < path.length(); ++i ) {
        if( path[i] == '/' ) {
            if( !lastWasSlash ) {
                lastWasSlash = true;
                s.append( "/" );
            }
        }
        else {
            lastWasSlash = false;
            s.append( path[i] );
        }
    }

    return s;
}


K3b::Version K3b::kernelVersion()
{
    // initialize kernel version
    K3b::Version v;
    utsname unameinfo;
    if( ::uname(&unameinfo) == 0 ) {
        v = QString::fromLocal8Bit( unameinfo.release );
        qDebug() << "kernel version: " << v;
    }
    else
        qCritical() << "could not determine kernel version." ;
    return v;
}


K3b::Version K3b::simpleKernelVersion()
{
    return kernelVersion().simplify();
}


QString K3b::systemName()
{
    QString v;
    utsname unameinfo;
    if( ::uname(&unameinfo) == 0 ) {
        v = QString::fromLocal8Bit( unameinfo.sysname );
    }
    else
        qCritical() << "could not determine system name." ;
    return v;
}


bool K3b::kbFreeOnFs( const QString& path, unsigned long& size, unsigned long& avail )
{
    const QStorageInfo fs(path);
    if ( fs.isValid() ) {
        size = fs.bytesTotal()/1024;
        avail = fs.bytesFree()/1024;
        return true;
    }
    else {
        return false;
    }
}


KIO::filesize_t K3b::filesize( const QUrl& url )
{
    KIO::filesize_t fSize = 0;
    if( url.isLocalFile() ) {
        QFileInfo fi( url.toLocalFile() );
        fSize = fi.size();
    }
    else {
        KIO::UDSEntry uds;
        KIO::StatJob* statJob = KIO::stat( url, KIO::HideProgressInfo );
        if (statJob->exec())
            uds = statJob->statResult();
        fSize = uds.numberValue( KIO::UDSEntry::UDS_SIZE );
    }

    return fSize;
}


KIO::filesize_t K3b::imageFilesize( const QUrl& url )
{
    KIO::filesize_t size = K3b::filesize( url );
    bool exists = true;
    for(int cnt = 0; exists; ++cnt)
    {
        QUrl nextUrl( url );
        nextUrl.setPath(nextUrl.path() + '.' + QString::number(cnt).rightJustified( 3, '0' ));
        KIO::StatJob* statJob = KIO::stat(nextUrl, KIO::StatJob::SourceSide, KIO::StatDefaultDetails, KIO::HideProgressInfo);
        if (statJob->exec())
            size += K3b::filesize(nextUrl);
        else
            exists = false;
    }
    return size;
}


QString K3b::cutFilename( const QString& name, int len )
{
    if( name.length() > len ) {
        QString ret = name;

        // determine extension (we think of an extension to be at most 5 chars in length)
        int pos = name.indexOf( '.', -6 );
        if( pos > 0 )
            len -= (name.length() - pos);

        ret.truncate( len );

        if( pos > 0 )
            ret.append( name.mid( pos ) );

        return ret;
    }
    else
        return name;
}


QString K3b::removeFilenameExtension( const QString& name )
{
    QString v = name;
    int dotpos = v.lastIndexOf( '.' );
    if( dotpos > 0 )
        v.truncate( dotpos );
    return v;
}


QString K3b::appendNumberToFilename( const QString& name, int num, unsigned int maxlen )
{
    // determine extension (we think of an extension to be at most 5 chars in length)
    QString result = name;
    QString ext;
    int pos = name.indexOf( '.', -6 );
    if( pos > 0 ) {
        ext = name.mid(pos);
        result.truncate( pos );
    }

    ext.prepend( QString::number(num) );
    result.truncate( maxlen - ext.length() );

    return result + ext;
}


bool K3b::plainAtapiSupport()
{
    // FIXME: what about BSD?
    return ( K3b::simpleKernelVersion() >= K3b::Version( 2, 5, 40 ) );
}


bool K3b::hackedAtapiSupport()
{
    // IMPROVEME!!!
    // FIXME: since when does the kernel support this?
    return ( K3b::simpleKernelVersion() >= K3b::Version( 2, 4, 0 ) );
}


QString K3b::externalBinDeviceParameter( K3b::Device::Device* dev, const K3b::ExternalBin* bin )
{
    Q_UNUSED( bin );
    return dev->blockDeviceName();
}


K3b::WritingApp K3b::writingAppFromString( const QString& s )
{
    if( s.toLower() == "cdrdao" )
        return K3b::WritingAppCdrdao;
    else if( s.toLower() == "cdrecord" )
        return K3b::WritingAppCdrecord;
    else if( s.toLower() == "growisofs" )
        return K3b::WritingAppGrowisofs;
    else if( s.toLower() == "dvd+rw-format" )
        return K3b::WritingAppDvdRwFormat;
    else if (s.toLower() == "cdrskin")
        return K3b::WritingAppCdrskin;
    else
        return K3b::WritingAppAuto;
}


QString K3b::writingAppToString( K3b::WritingApp app )
{
    switch( app ) {
    case WritingAppCdrecord:
        return "cdrecord";
    case WritingAppCdrdao:
        return "cdrdao";
    case WritingAppGrowisofs:
        return "growisofs";
    case WritingAppDvdRwFormat:
        return "dvd+rw-format";
    default:
        return "auto";
    }
}


QString K3b::writingModeString( K3b::WritingModes modes )
{
    if( modes == WritingModeAuto )
        return i18n("Auto");
    else
        return K3b::Device::writingModeString( ( int )modes );
}


QString K3b::resolveLink( const QString& file )
{
    QFileInfo f( file );
    return f.canonicalFilePath();
}


QUrl K3b::convertToLocalUrl( const QUrl& url )
{
    if( !url.isLocalFile() ) {
        KIO::StatJob* statJob = KIO::mostLocalUrl( url, KIO::HideProgressInfo );
        QUrl result;
        QObject::connect( statJob, &KJob::result, [&](KJob*) {
            if( statJob->error() == KJob::NoError )
                result = statJob->mostLocalUrl();
        } );
        statJob->exec();
        return result;
    }

    return url;
}


QList<QUrl> K3b::convertToLocalUrls( const QList<QUrl>& urls )
{
    QList<QUrl> r;
    for( QList<QUrl>::const_iterator it = urls.constBegin(); it != urls.constEnd(); ++it )
        r.append( convertToLocalUrl( *it ) );
    return r;
}


qint16 K3b::fromLe16( char* data )
{
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    return swapByteOrder( *((qint16*)data) );
#else
    return *((qint16*)data);
#endif
}


qint32 K3b::fromLe32( char* data )
{
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    return swapByteOrder( *((qint32*)data) );
#else
    return *((qint32*)data);
#endif
}


qint64 K3b::fromLe64( char* data )
{
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    return swapByteOrder( *((qint64*)data) );
#else
    return *((qint64*)data);
#endif
}


QString K3b::findExe( const QString& name )
{
    // first we search the path
    QString bin = QStandardPaths::findExecutable( name );

    // then go on with our own little list
    if( bin.isEmpty() )
        bin = QStandardPaths::findExecutable( name, k3bcore->externalBinManager()->searchPath() );

    return bin;
}


bool K3b::isMounted( K3b::Device::Device* dev )
{
    if( !dev )
        return false;
    else
        return( KMountPoint::currentMountPoints().findByDevice( dev->blockDeviceName() ).data() != nullptr );
}


bool K3b::unmount( K3b::Device::Device* dev )
{
    if( !dev )
        return false;

    Solid::StorageAccess *sa = dev->solidStorage();
    if ( sa && sa->teardown() ){
        return true;
    }

    QString mntDev = dev->blockDeviceName();

    // first try to unmount it the standard way
    KIO::SimpleJob* unmountJob = KIO::unmount( mntDev );
    bool result = true;
    QObject::connect( unmountJob, &KJob::result, [&](KJob* job){ result = ( job->error() != KJob::NoError ); } );
    if( unmountJob->exec() && result )
        return true;

    QString mntPath;
    if ( KMountPoint::Ptr mp = KMountPoint::currentMountPoints().findByDevice( dev->blockDeviceName() ) ) {
        mntPath = mp->mountPoint();
    }
    if ( mntPath.isEmpty() ) {
        mntPath = dev->blockDeviceName();
    }

    QString umountBin = K3b::findExe( "umount" );
    if( !umountBin.isEmpty() ) {
        KProcess p;
        p << umountBin;
        p << "-l"; // lazy unmount
        p << mntPath;
        p.start();
        if (p.waitForFinished(-1))
          return true;
    }

    // now try pmount
    QString pumountBin = K3b::findExe( "pumount" );
    if( !pumountBin.isEmpty() ) {
        KProcess p;
        p << pumountBin;
        p << "-l"; // lazy unmount
        p << mntPath;
        p.start();
        return p.waitForFinished(-1);
    }
    else {
        return false;
    }
}


bool K3b::mount( K3b::Device::Device* dev )
{
    if( !dev )
        return false;

    QString mntDev = dev->blockDeviceName();

    // first try to mount it the standard way
    KIO::SimpleJob* mountJob = KIO::mount( true, QByteArray(), mntDev, QString() );
    bool result = true;
    QObject::connect( mountJob, &KJob::result, [&](KJob* job){ result = ( job->error() != KJob::NoError ); } );
    if( mountJob->exec() && result )
        return true;

    Solid::StorageAccess* sa = dev->solidStorage();
    if ( sa && sa->setup() ) {
        return true;
    }

    // now try pmount
    QString pmountBin = K3b::findExe( "pmount" );
    if( !pmountBin.isEmpty() ) {
        KProcess p;
        p << pmountBin;
        p << mntDev;
        p.start();
        return p.waitForFinished(-1);
    }

    // and the most simple one
    QString mountBin = K3b::findExe( "mount" );
    if( !mountBin.isEmpty() ) {
        KProcess p;
        p << mountBin;
        p << mntDev;
        p.start();
        return p.waitForFinished(-1);
    }

    return false;
}


bool K3b::eject( K3b::Device::Device* dev )
{
    if( K3b::isMounted( dev ) )
        K3b::unmount( dev );

    if ( dev->solidDevice().as<Solid::OpticalDrive>()->eject() ||
         dev->eject() ) {
        // to be on the safe side, especially with respect to the EmptyDiscWaiter
        // we reset the device in the cache.
        k3bcore->mediaCache()->resetDevice( dev );
        return true;
    }
    else {
        return false;
    }
}


K3b::Device::SpeedMultiplicator K3b::speedMultiplicatorForMediaType( K3b::Device::MediaType mediaType )
{
    if ( mediaType & K3b::Device::MEDIA_DVD_ALL ) {
        return K3b::Device::SPEED_FACTOR_DVD;
    }
    else if ( mediaType & K3b::Device::MEDIA_BD_ALL ) {
        return K3b::Device::SPEED_FACTOR_BD;
    }
    else {
        return K3b::Device::SPEED_FACTOR_CD;
    }
}


QString K3b::formatWritingSpeedFactor( int speed, K3b::Device::MediaType mediaType, SpeedFormat speedFormat )
{
    const int speedFactor = speedMultiplicatorForMediaType( mediaType );
    int normalizedSpeed = speed;
    int diff = normalizedSpeed%speedFactor;
    if ( diff < 5 )
        normalizedSpeed = speed-diff;
    else if ( diff > speedFactor-5 )
        normalizedSpeed = speed+speedFactor-diff;

    // speed may be a float number. example: DVD+R(W): 2.4x
    if ( mediaType & K3b::Device::MEDIA_DVD_ALL &&
         normalizedSpeed%speedFactor > 0 &&
         speedFormat != SpeedFormatInteger ) {
         return QString::number( ( float )normalizedSpeed/( float )speedFactor, 'f', 1 );
    }
    else {
        return QString::number( normalizedSpeed/speedFactor );
    }
}


bool K3b::IsOverburnAllowed( const K3b::Msf& projectSize, const K3b::Msf& capacity )
{
    return IsOverburnAllowed( projectSize, capacity, Msf() );
}


bool K3b::IsOverburnAllowed( const Msf& projectSize, const Msf& capacity, const Msf& usedCapacity )
{
    return( k3bcore->globalSettings()->overburn() &&
        (projectSize + usedCapacity) <= ( capacity.lba() + capacity.lba() / 4 ) ); // 25% tolerance in overburn mode
}


QDebug& K3b::operator<<( QDebug& dbg, K3b::WritingMode mode )
{
    return dbg << K3b::Device::WritingMode( mode );
}


QDebug& K3b::operator<<( QDebug& dbg, K3b::WritingModes modes )
{
    return dbg << K3b::Device::WritingModes( ( int )modes );
}
