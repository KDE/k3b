/*

    SPDX-FileCopyrightText: 2005 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef _videodvd_H_
#define _videodvd_H_

#include <QString>

#include "k3biso9660.h"
#include "k3biso9660backend.h"

#include <KIO/SlaveBase>

class Iso9660Entry;
class Iso9660;
namespace K3b {
    namespace Device {
        class DeviceManager;
    }
}

class kio_videodvdProtocol : public KIO::SlaveBase
{
public:
    kio_videodvdProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
    ~kio_videodvdProtocol() override;

    void mimetype(const QUrl& url) override;
    void stat(const QUrl& url) override;
    void get(const QUrl& url) override;
    void listDir(const QUrl& url) override;

private:
    K3b::Iso9660* openIso( const QUrl&, QString& plainIsoPath );
    KIO::UDSEntry createUDSEntry( const K3b::Iso9660Entry* e ) const;
    void listVideoDVDs();

    static K3b::Device::DeviceManager* s_deviceManager;
    static int s_instanceCnt;
};

#endif
