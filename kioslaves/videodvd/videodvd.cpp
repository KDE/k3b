/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2010 Michal Malek <michalm@jabster.pl>
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

#include <config-k3b.h>

#include <qdatetime.h>
#include <qbitarray.h>

#include <kdebug.h>
#include <KComponentData>
#include <kglobal.h>
#include <klocale.h>
#include <kurl.h>

#include <stdlib.h>

#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3biso9660.h"
#include "k3biso9660backend.h"

#include "videodvd.h"

using namespace KIO;

extern "C"
{
    KDE_EXPORT int kdemain( int argc, char **argv )
    {
        KComponentData componentData( "kio_videodvd" );

        kDebug(7101) << "*** Starting kio_videodvd ";

        if (argc != 4)
        {
            kDebug(7101) << "Usage: kio_videodvd  protocol domain-socket1 domain-socket2";
            exit(-1);
        }

        kio_videodvdProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        kDebug(7101) << "*** kio_videodvd Done";
        return 0;
    }

    bool isRootDirectory( const KUrl& url )
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
    : SlaveBase("kio_videodvd", pool_socket, app_socket)
{
    kDebug() << "kio_videodvdProtocol::kio_videodvdProtocol()";
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
    kDebug() << "kio_videodvdProtocol::~kio_videodvdProtocol()";
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
    uds.insert(KIO::UDSEntry::UDS_NAME,e->name());
    uds.insert(KIO::UDSEntry::UDS_ACCESS, e->permissions());
    uds.insert(KIO::UDSEntry::UDS_CREATION_TIME, e->date());
    uds.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME,e->date());

    if( e->isDirectory() )
    {
        uds.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        uds.insert(KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory");
    }
    else
    {
        const K3b::Iso9660File* file = static_cast<const K3b::Iso9660File*>( e );
        uds.insert(KIO::UDSEntry::UDS_SIZE,file->size());
        uds.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
        QString iconName;
        if( e->name().endsWith( "VOB" ) )
            iconName = "video/mpeg";
        else
            iconName = "unknown";
        uds.insert(KIO::UDSEntry::UDS_MIME_TYPE, iconName);
    }

    return uds;
}


// FIXME: remember the iso instance for quicker something and search for the videodvd
//        in the available devices.
K3b::Iso9660* kio_videodvdProtocol::openIso( const KUrl& url, QString& plainIsoPath )
{
    // get the volume id from the url
    QString volumeId = url.path().section( '/', 1, 1 );

    kDebug() << "(kio_videodvdProtocol) searching for Video dvd: " << volumeId;


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
                plainIsoPath = url.path().section( "/", 2, -1 ) + "/";
                kDebug() << "(kio_videodvdProtocol) using iso path: " << plainIsoPath;
                return iso;
            }
            delete iso;
        }
    }

    error( ERR_SLAVE_DEFINED, i18n("No Video DVD found") );
    return 0;
}


void kio_videodvdProtocol::get(const KUrl& url )
{
    kDebug() << "kio_videodvd::get(const KUrl& url)";

    QString isoPath;
    if( K3b::Iso9660* iso = openIso( url, isoPath ) )
    {
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

            delete iso;

            data(QByteArray()); // empty array means we're done sending the data

            if( read == 0 )
                finished();
            else
                error( ERR_SLAVE_DEFINED, i18n("Read error.") );
        }
        else
            error( ERR_DOES_NOT_EXIST, url.path() );
    }
}


void kio_videodvdProtocol::listDir( const KUrl& url )
{
    if( isRootDirectory( url ) ) {
#ifdef Q_OS_WIN32
    kDebug() << "fix of root path required";
#endif
        listVideoDVDs();
    }
    else {
        QString isoPath;
        K3b::Iso9660* iso = openIso( url, isoPath );
        if( iso ) {
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
                    finished();
                }
                else {
                    error( ERR_CANNOT_ENTER_DIRECTORY, url.path() );
                }
            }
            else {
                error( ERR_CANNOT_ENTER_DIRECTORY, url.path() );
            }

            // for testing we always do the whole thing
            delete iso;
        }
    }
}


void kio_videodvdProtocol::listVideoDVDs()
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
                uds.insert( KIO::UDSEntry::UDS_NAME,iso.primaryDescriptor().volumeId );
                uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
                uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory" );
                uds.insert( KIO::UDSEntry::UDS_ICON_NAME, "media-optical-video" );
                uds.insert( KIO::UDSEntry::UDS_SIZE, iso.primaryDescriptor().volumeSetSize );

                udsl.append( uds );
                listEntries( udsl );
            }
        }
    }

    if( !udsl.isEmpty() ) {
        finished();
    }
    else {
        error( ERR_SLAVE_DEFINED, i18n("No Video DVD found") );
    }
}


void kio_videodvdProtocol::stat( const KUrl& url )
{
    if( isRootDirectory( url ) ) {
#ifdef Q_OS_WIN32
    kDebug() << "fix root path detection";
#endif
        //
        // stat the root path
        //
        KIO::UDSEntry uds;
        uds.insert( KIO::UDSEntry::UDS_NAME, url.path() );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory" );

        statEntry( uds );
        finished();
    }
    else {
        QString isoPath;
        K3b::Iso9660* iso = openIso( url, isoPath );
        if( iso ) {
            const K3b::Iso9660Entry* e = iso->firstIsoDirEntry()->entry( isoPath );
            if( e ) {
                statEntry( createUDSEntry( e ) );
                finished();
            }
            else
                error( ERR_DOES_NOT_EXIST, url.path() );

            delete iso;
        }
    }
}


// FIXME: when does this get called? It seems not to be used for the files.
// This is called by KIO::mimetype, which is called e.g. when dropping an item onto
// the "places" widget in the file dialog. Indeed not much used these days.
// Note that you can also implement it as a get() without the "send the data"
// part of it. (David)
void kio_videodvdProtocol::mimetype( const KUrl& url )
{
    if( isRootDirectory( url ) ) {
        error( ERR_UNSUPPORTED_ACTION, KIO::unsupportedActionErrorString("videodvd", CMD_MIMETYPE) );
        return;
    }

    QString isoPath;
    K3b::Iso9660* iso = openIso( url, isoPath );
    if( iso ) {
        const K3b::Iso9660Entry* e = iso->firstIsoDirEntry()->entry( isoPath );
        if( e ) {
            if( e->isDirectory() ) {
                KIO::SlaveBase::mimeType( "inode/directory" );
            }
            else if( e->name().endsWith( ".VOB" ) ) {
                KIO::SlaveBase::mimeType( "video/mpeg" );
            }
            else {
                // send some data
                const K3b::Iso9660File* file = static_cast<const K3b::Iso9660File*>( e );
                QByteArray buffer( 10*2048, '\n' );
                int read = file->read( 0, buffer.data(), buffer.size() );
                if( read > 0 )
                {
                    buffer.resize( read );
                    data(buffer);
                    data(QByteArray());
                    finished();
                    // FIXME: do we need to emit finished() after emitting the end of data()?
                }
                else
                    error( ERR_SLAVE_DEFINED, i18n("Read error.") );
            }
        }
        delete iso;
    }
}
