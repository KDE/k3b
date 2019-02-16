/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_MD5_JOB_H_
#define _K3B_MD5_JOB_H_

#include "k3b_export.h"
#include "k3bjob.h"
#include <QByteArray>

class QIODevice;

namespace K3b {
    namespace Device {
        class Device;
    }

    class Iso9660File;

    class LIBK3B_EXPORT Md5Job : public Job
    {
        Q_OBJECT

    public:
        explicit Md5Job( JobHandler* jh , QObject* parent = 0 );
        ~Md5Job() override;

		QByteArray hexDigest();
		QByteArray base64Digest();

    public Q_SLOTS:
        void start() override;
        void stop();
        void cancel() override;

        // FIXME: read from QIODevice and thus add FileSplitter support

        /**
         * read from a file.
         *
         * Be aware that the Md5Job uses FileSplitter to read split
         * images. In the future this will be changed with the introduction
         * of a setIODevice method.
         */
        void setFile( const QString& filename );

        /**
         * read from an iso9660 file
         */
        void setFile( const Iso9660File* );

        /**
         * read from a device
         * This should be used in combination with setMaxReadSize
         */
        void setDevice( Device::Device* dev );

        /**
         * read from the opened QIODevice.
         * One needs to set the max read length or call stop()
         * to finish calculation.
         */
        void setIODevice( QIODevice* ioDev );

        /**
         * Set the maximum bytes to read.
         */
        void setMaxReadSize( qint64 );

    private Q_SLOTS:
        void slotUpdate();

    private:
        void setupFdNotifier();
        void stopAll();

        class Private;
        Private* const d;
    };
}

#endif
