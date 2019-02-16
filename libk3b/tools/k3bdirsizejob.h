/*
 *
 * Copyright (C) 2007-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DIR_SIZE_JOB_H_
#define _K3B_DIR_SIZE_JOB_H_

#include "k3bthreadjob.h"
#include <KIO/Global>
#include <QUrl>

#include "k3b_export.h"

namespace K3b {
    /**
     * DirSizeJob is a replacement for KDirSize which allows
     * a much finer grained control over what is counted and how.
     * Additionally it uses threading for enhanced speed.
     *
     * For now DirSizeJob only works on local urls.
     */
    class LIBK3B_EXPORT DirSizeJob : public ThreadJob
    {
        Q_OBJECT

    public:
        explicit DirSizeJob( QObject* parent = 0 );
        ~DirSizeJob() override;

        KIO::filesize_t totalSize() const;

        /**
         * Does also include symlinks to files, devices, and fifos
         */
        KIO::filesize_t totalFiles() const;

        /**
         * Total number of counted dirs. This does also
         * include the first dirs the job was started with.
         * Does also include symlinks to dirs.
         */
        KIO::filesize_t totalDirs() const;

        /**
         * Includes symlinks to files and folders
         */
        KIO::filesize_t totalSymlinks() const;

    public Q_SLOTS:
        void setUrls( const QList<QUrl>& urls );
        void setFollowSymlinks( bool );

    private:
        bool run() override;
        bool countDir( const QString& dir );
        bool countFiles( const QStringList& l, const QString& dir );

        class Private;
        Private* const d;
    };
}

#endif
