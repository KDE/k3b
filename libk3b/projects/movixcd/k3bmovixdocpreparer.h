/*
    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MOVIX_DOC_PREPARER_H_
#define _K3B_MOVIX_DOC_PREPARER_H_

#include "k3bjob.h"

namespace K3b {
    class MovixDoc;
    class FileItem;
    class DirItem;

    /**
     * This class creates the needed eMovix structures in an eMovix doc
     * and removes them after creating the image.
     */
    class MovixDocPreparer : public Job
    {
        Q_OBJECT

    public:
        explicit MovixDocPreparer( MovixDoc* doc, JobHandler*, QObject* parent = 0 );
        ~MovixDocPreparer() override;

        MovixDoc* doc() const;

        bool createMovixStructures();
        void removeMovixStructures();

    public Q_SLOTS:
        /**
         * use createMovixStructures and removeMovixStructures instead.
         */
        void start() override;

        /**
         * Useless since this job works synchronously
         */
        void cancel() override;

    private:
        bool writePlaylistFile();
        bool writeIsolinuxConfigFile( const QString& );
        bool writeMovixRcFile();
        bool addMovixFiles();
        bool addMovixFilesNew();
        FileItem* createItem( const QString& localPath, const QString& docPath );
        DirItem* createDir( const QString& docPath );

        class Private;
        Private* d;
    };
}

#endif
