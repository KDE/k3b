/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_FILECOMPILATION_SIZE_HANDLER_H_
#define _K3B_FILECOMPILATION_SIZE_HANDLER_H_

#include "k3bmsf.h"
#include <KIO/Global>

namespace K3b {
    class DataItem;

    /**
     * This class maintains a map of indoes and the number
     * of files in the doc that belong to that inode.
     * This way a more accurate size calculation is possible
     *
     * It has to be noted that the sizes of the directories
     * are only locally true. That means that in some cases
     * the root directory of the project may show a much
     * higher size than calculated by this class.
     */
    class FileCompilationSizeHandler
    {
    public:
        FileCompilationSizeHandler();
        ~FileCompilationSizeHandler();

        /**
         * This does NOT equal blocks() * 2048.
         * This is the sum of the actual file sizes.
         */
        const KIO::filesize_t& size( bool followSymlinks = false ) const;

        /**
         * Number of blocks the files will occupy.
         */
        const Msf& blocks( bool followSymlinks = false ) const;

        /**
         * This will increase the counter for the inode of
         * the file in url and update the totel size.
         */
        void addFile( DataItem* );

        /**
         * This will decrease the counter for the inode of
         * the file in url and update the totel size.
         */
        void removeFile( DataItem* );

        void clear();

    private:
        class Private;
        Private* d_symlinks;
        Private* d_noSymlinks;
    };
}

#endif
