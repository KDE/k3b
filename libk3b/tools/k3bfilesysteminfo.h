/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_FILE_SYSTEM_INFO_H_
#define _K3B_FILE_SYSTEM_INFO_H_

#include "k3b_export.h"

#include <QString>

namespace K3b {
    class LIBK3B_EXPORT FileSystemInfo
    {
    public:
        FileSystemInfo();
        explicit FileSystemInfo( const QString& path );
        FileSystemInfo( const FileSystemInfo& );
        ~FileSystemInfo();

        QString path() const;
        void setPath( const QString& path );

        enum FileSystemType {
            FS_UNKNOWN,
            FS_FAT
            // FIXME: add way more file system types
        };

        FileSystemType type() const;

        /**
         * Ensures that the file path does not contain
         * any invalid chars.
         *
         * For now it only replaces characters like * or [
         * on FAT file systems.
         */
        QString fixupPath( const QString& );

    private:
        class Private;
        Private* d;
    };
}

#endif
