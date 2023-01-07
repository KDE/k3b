/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _videodvd_H_
#define _videodvd_H_

#include <QString>

#include "k3biso9660.h"
#include "k3biso9660backend.h"

#include <KIO/WorkerBase>

#include <memory>

namespace K3b {
    namespace Device {
        class DeviceManager;
    }
}

class kio_videodvdProtocol : public KIO::WorkerBase
{
public:
    kio_videodvdProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
    ~kio_videodvdProtocol() override;

    KIO::WorkerResult mimetype(const QUrl& url) override;
    KIO::WorkerResult stat(const QUrl& url) override;
    KIO::WorkerResult get(const QUrl& url) override;
    KIO::WorkerResult listDir(const QUrl& url) override;

private:
    KIO::WorkerResult openIso( const QUrl& url, std::unique_ptr<K3b::Iso9660>* iso, QString* plainIsoPath );
    KIO::UDSEntry createUDSEntry( const K3b::Iso9660Entry* e ) const;
    KIO::WorkerResult listVideoDVDs();

    static K3b::Device::DeviceManager* s_deviceManager;
    static int s_instanceCnt;
};

#endif
