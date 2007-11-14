/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_MD5_JOB_H_
#define _K3B_MD5_JOB_H_

#include <k3bjob.h>
#include <qbytearray.h>
#include "k3b_export.h"

namespace K3bDevice {
    class Device;
}

class K3bIso9660File;


class LIBK3B_EXPORT K3bMd5Job : public K3bJob
{
    Q_OBJECT

public:
    K3bMd5Job( K3bJobHandler* jh , QObject* parent = 0 );
    ~K3bMd5Job();

    QByteArray hexDigest();
    QByteArray base64Digest();

public Q_SLOTS:
    void start();
    void stop();
    void cancel();

    // FIXME: read from QIODevice and thus add K3bFileSplitter support

    /**
     * read from a file.
     *
     * Be aware that the K3bMd5Job uses K3bFileSplitter to read splitted
     * images. In the future this will be changed with the introduction
     * of a setIODevice method.
     */
    void setFile( const QString& filename );

    /**
     * read from an iso9660 file
     */
    void setFile( const K3bIso9660File* );

    /**
     * read from a device
     * This should be used in combination with setMaxReadSize
     */
    void setDevice( K3bDevice::Device* dev );

    /**
     * read from the opened file descriptor.
     * One needs to set the max read length or call stop()
     * to finish calculation.
     */
    void setFd( int fd );

    /**
     * Set the maximum bytes to read.
     */
    void setMaxReadSize( qint64 );

private Q_SLOTS:
    void slotUpdate();

private:
    void setupFdNotifier();
    void stopAll();

    class K3bMd5JobPrivate;
    K3bMd5JobPrivate* d;
};

#endif
