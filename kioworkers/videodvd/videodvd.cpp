/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "videodvd.h"

#include <config-k3b.h>
#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "videodvd_export.h"
#include "videodvd_log.h"
#include "videodvd_i18n.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QBitArray>

#include <stdlib.h>

namespace
{
    const int CMD_MIMETYPE = 70; // Should be declared in KIOCore/KIO/Global, but it's missing. Why?
} // namespace

using namespace KIO;

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.videodvd" FILE "videodvd.json")
};

extern "C"
{
    VIDEODVD_EXPORT int kdemain( int argc, char **argv )
    {
        QCoreApplication app(argc, argv);
        QCoreApplication::setApplicationName( "kio_videodvd" );

        qCDebug(KIO_VIDEODVD_LOG) << "Starting";

        if (argc != 4)
        {
            fprintf(stderr, "Usage: kio_videodvd protocol domain-socket1 domain-socket2\n");
            exit(-1);
        }

        kio_videodvdProtocol worker(argv[2], argv[3]);
        worker.dispatchLoop();

        qCDebug(KIO_VIDEODVD_LOG) << "Done";
        return 0;
    }

    bool isRootDirectory( const QUrl& url )
    {
        QString path = url.path();
        return( path.isEmpty() || path == "/" );
    }
}



// FIXME: Does it really make sense to use a static device manager? Are all instances
// of videodvd started in another process?
K3b::Device::DeviceManager* kio_videodvdProtocol::s_deviceManager = 0;
int kio_videodvdProtocol::s_instanceCnt = 0;

kio_videodvdProtocol::kio_videodvdProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
    : WorkerBase("kio_videodvd", pool_socket, app_socket)
{
    qCDebug(KIO_VIDEODVD_LOG) << "kio_videodvdProtocol::kio_videodvdProtocol()";
    if( !s_deviceManager )
    {
        s_deviceManager = new K3b::Device::DeviceManager();
        s_deviceManager->setCheckWritingModes( false );
        s_deviceManager->scanBus();
    }
    s_instanceCnt++;
}


kio_videodvdProtocol::~kio_videodvdProtocol()
{
    qCDebug(KIO_VIDEODVD_LOG) << "kio_videodvdProtocol::~kio_videodvdProtocol()";
    s_instanceCnt--;
    if( s_instanceCnt == 0 )
    {
        delete s_deviceManager;
        s_deviceManager = 0;
    }
}


KIO::UDSEntry kio_videodvdProtocol::createUDSEntry( const K3b::Iso9660Entry* e ) const
{
    KIO::UDSEntry uds;
    uds.fastInsert(KIO::UDSEntry::UDS_NAME,e->name());
    uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, e->permissions());
    uds.fastInsert(KIO::UDSEntry::UDS_CREATION_TIME, e->date());
    uds.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME,e->date());

    if( e->isDirectory() )
    {
        uds.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory");
    }
    else
    {
        const K3b::Iso9660File* file = static_cast<const K3b::Iso9660File*>( e );
        uds.fastInsert(KIO::UDSEntry::UDS_SIZE,file->size());
        uds.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
        QString iconName;
        if( e->name().endsWith( "VOB" ) )
            iconName = "video/mpeg";
        else
            iconName = "unknown";
        uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, iconName);
    }

    return uds;
}


// FIXME: remember the iso instance for quicker something and search for the videodvd
//        in the available devices.
KIO::WorkerResult kio_videodvdProtocol::openIso( const QUrl& url, std::unique_ptr<K3b::Iso9660>* isoPtr, QString* plainIsoPath )
{
    // get the volume id from the url
    QString volumeId = url.path().section( '/', 1, 1 );

    qCDebug(KIO_VIDEODVD_LOG) << "(kio_videodvdProtocol) searching for Video dvd: " << volumeId;


    // now search the devices for this volume id
    // FIXME: use the cache created in listVideoDVDs
    QList<K3b::Device::Device *> items(s_deviceManager->dvdReader());
    for( QList<K3b::Device::Device *>::const_iterator it = items.constBegin();
         it != items.constEnd(); ++it ) {
        K3b::Device::Device* dev = *it;
        K3b::Device::DiskInfo di = dev->diskInfo();

        // we search for a DVD with a single track.
        // this time let K3b::Iso9660 decide if we need dvdcss or not
        // FIXME: check for encryption and libdvdcss and report an error
        if( K3b::Device::isDvdMedia( di.mediaType() ) && di.numTracks() == 1 ) {
            K3b::Iso9660* iso = new K3b::Iso9660( dev );
            iso->setPlainIso9660( true );
            if( iso->open() /*&& iso->primaryDescriptor().volumeId == volumeId*/ ) {
                *plainIsoPath = url.path().section( '/', 2, -1 ) + '/';
                (*isoPtr).reset(iso);
                qCDebug(KIO_VIDEODVD_LOG) << "(kio_videodvdProtocol) using iso path: " << *plainIsoPath;
                return KIO::WorkerResult::pass();
            }
            delete iso;
        }
    }

    return KIO::WorkerResult::fail( ERR_WORKER_DEFINED, i18n("No Video DVD found") );
}


KIO::WorkerResult kio_videodvdProtocol::get(const QUrl& url )
{
    qCDebug(KIO_VIDEODVD_LOG) << "kio_videodvd::get(const QUrl& url)" << url;

    QString isoPath;
    std::unique_ptr<K3b::Iso9660> iso;
    const KIO::WorkerResult openIsoResult = openIso(url, &iso, &isoPath);
    if (!openIsoResult.success()) {
        return openIsoResult;
    }

    const K3b::Iso9660Entry* e = iso->firstIsoDirEntry()->entry( isoPath );
    if( e && e->isFile() )
    {
        const K3b::Iso9660File* file = static_cast<const K3b::Iso9660File*>( e );
        totalSize( file->size() );
        QByteArray buffer( 10*2048, '\n' );
        int read = 0;
        int cnt = 0;
        KIO::filesize_t totalRead = 0;
        while( (read = file->read( totalRead, buffer.data(), buffer.size() )) > 0 )
        {
            buffer.resize( read );
            data(buffer);
            ++cnt;
            totalRead += read;
            if( cnt == 10 )
            {
                cnt = 0;
                processedSize( totalRead );
            }
        }

        data(QByteArray()); // empty array means we're done sending the data

        if( read == 0 ) {
            return KIO::WorkerResult::pass();
        }

        return KIO::WorkerResult::fail( ERR_WORKER_DEFINED, i18n("Read error.") );
    }

    return KIO::WorkerResult::fail( ERR_DOES_NOT_EXIST, url.path() );
}


KIO::WorkerResult kio_videodvdProtocol::listDir( const QUrl& url )
{
    qCDebug(KIO_VIDEODVD_LOG) << "kio_videodvd::listDir(const QUrl& url)" << url;

    if( isRootDirectory( url ) ) {
#ifdef Q_OS_WIN32
    qCWarning(KIO_VIDEODVD_LOG) << "fix of root path required";
#endif
        return listVideoDVDs();
    }

    QString isoPath;
    std::unique_ptr<K3b::Iso9660> iso;
    const KIO::WorkerResult openIsoResult = openIso(url, &iso, &isoPath);
    if (!openIsoResult.success()) {
        return openIsoResult;
    }

    const K3b::Iso9660Directory* mainDir = iso->firstIsoDirEntry();
    const K3b::Iso9660Entry* e = mainDir->entry( isoPath );
    if( e ) {
        if( e->isDirectory() ) {
            const K3b::Iso9660Directory* dir = static_cast<const K3b::Iso9660Directory*>(e);
            QStringList el = dir->entries();
            el.removeOne( "." );
            el.removeOne( ".." );
            UDSEntryList udsl;
            for( QStringList::const_iterator it = el.constBegin(); it != el.constEnd(); ++it )
                udsl.append( createUDSEntry( dir->entry( *it ) ) );
            listEntries( udsl );
            return KIO::WorkerResult::pass();
        }

        return KIO::WorkerResult::fail( ERR_CANNOT_ENTER_DIRECTORY, url.path() );
    }

    return KIO::WorkerResult::fail( ERR_CANNOT_ENTER_DIRECTORY, url.path() );
}


KIO::WorkerResult kio_videodvdProtocol::listVideoDVDs()
{
    UDSEntryList udsl;

    QList<K3b::Device::Device *> items(s_deviceManager->dvdReader());
    for( QList<K3b::Device::Device *>::const_iterator it = items.constBegin();
         it != items.constEnd(); ++it ) {
        K3b::Device::Device* dev = *it;
        K3b::Device::DiskInfo di = dev->diskInfo();

        // we search for a DVD with a single track.
        if( K3b::Device::isDvdMedia( di.mediaType() ) && di.numTracks() == 1 ) {
            //
            // now do a quick check for VideoDVD.
            // - no dvdcss for speed
            // - only a check for the VIDEO_TS dir
            //
            K3b::Iso9660 iso( new K3b::Iso9660DeviceBackend(dev) );
            iso.setPlainIso9660( true );
            if( iso.open() && iso.firstIsoDirEntry()->entry( "VIDEO_TS" ) != 0 ) {
                // FIXME: cache the entry for speedup

                UDSEntry uds;
                uds.fastInsert( KIO::UDSEntry::UDS_NAME,iso.primaryDescriptor().volumeId );
                uds.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
                uds.fastInsert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory" );
                uds.fastInsert( KIO::UDSEntry::UDS_ICON_NAME, "media-optical-video" );
                uds.fastInsert( KIO::UDSEntry::UDS_SIZE, iso.primaryDescriptor().volumeSetSize );

                udsl.append( uds );
                listEntries( udsl );
            }
        }
    }

    if( !udsl.isEmpty() ) {
        return KIO::WorkerResult::pass();
    }
    return KIO::WorkerResult::fail( ERR_WORKER_DEFINED, i18n("No Video DVD found") );
}


KIO::WorkerResult kio_videodvdProtocol::stat( const QUrl& url )
{
    qCDebug(KIO_VIDEODVD_LOG) << "kio_videodvd::stat(const QUrl& url)" << url;

    if( isRootDirectory( url ) ) {
#ifdef Q_OS_WIN32
    qCWarning(KIO_VIDEODVD_LOG) << "fix root path detection";
#endif
        //
        // stat the root path
        //
        KIO::UDSEntry uds;
        uds.fastInsert( KIO::UDSEntry::UDS_NAME, url.path() );
        uds.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.fastInsert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory" );

        statEntry( uds );
        return KIO::WorkerResult::pass();
    }

    QString isoPath;
    std::unique_ptr<K3b::Iso9660> iso;
    const KIO::WorkerResult openIsoResult = openIso(url, &iso, &isoPath);
    if (!openIsoResult.success()) {
        return openIsoResult;
    }

    const K3b::Iso9660Entry* e = iso->firstIsoDirEntry()->entry( isoPath );
    if( e ) {
        statEntry( createUDSEntry( e ) );
        return KIO::WorkerResult::pass();
    }
    return KIO::WorkerResult::fail( ERR_DOES_NOT_EXIST, url.path() );
}


// FIXME: when does this get called? It seems not to be used for the files.
// This is called by KIO::mimetype, which is called e.g. when dropping an item onto
// the "places" widget in the file dialog. Indeed not much used these days.
// Note that you can also implement it as a get() without the "send the data"
// part of it. (David)
KIO::WorkerResult kio_videodvdProtocol::mimetype( const QUrl& url )
{
    qCDebug(KIO_VIDEODVD_LOG) << "kio_videodvd::mimetype(const QUrl& url)" << url;

    if( isRootDirectory( url ) ) {
        return KIO::WorkerResult::fail( ERR_UNSUPPORTED_ACTION, KIO::unsupportedActionErrorString("videodvd", CMD_MIMETYPE) );
    }

    QString isoPath;
    std::unique_ptr<K3b::Iso9660> iso;
    const KIO::WorkerResult openIsoResult = openIso(url, &iso, &isoPath);
    if (!openIsoResult.success()) {
        return openIsoResult;
    }

    const K3b::Iso9660Entry* e = iso->firstIsoDirEntry()->entry( isoPath );
    if( e ) {
        if( e->isDirectory() ) {
            mimeType( "inode/directory" );
            return KIO::WorkerResult::pass();
        }
        if( e->name().endsWith( ".VOB" ) ) {
            mimeType( "video/mpeg" );
            return KIO::WorkerResult::pass();
        }

        // send some data
        const K3b::Iso9660File* file = static_cast<const K3b::Iso9660File*>( e );
        QByteArray buffer( 10*2048, '\n' );
        int read = file->read( 0, buffer.data(), buffer.size() );
        if( read > 0 )
        {
            buffer.resize( read );
            data(buffer);
            data(QByteArray());
            return KIO::WorkerResult::pass();
            // FIXME: do we need to emit finished() after emitting the end of data()?
        }
        return KIO::WorkerResult::fail( ERR_WORKER_DEFINED, i18n("Read error.") );
    }
    return KIO::WorkerResult::fail( ERR_DOES_NOT_EXIST, url.path() );
}

#include "videodvd.moc"
