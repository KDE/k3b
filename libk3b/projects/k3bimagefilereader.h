/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_IMAGE_FILE_READER_H_
#define _K3B_IMAGE_FILE_READER_H_

#include "k3b_export.h"
#include <QString>

namespace K3b {
    class LIBK3B_EXPORT ImageFileReader
    {
    public:
        ImageFileReader();
        virtual ~ImageFileReader();

        /**
         * Open a file. In most cases the TOC file
         */
        void openFile( const QString& filename );

        virtual bool isValid() const;

        /**
         * Return the current set filename;
         */
        QString filename() const;

        /**
         * returns the name of the corresponding image file.
         */
        virtual QString imageFilename() const;

    protected:
        virtual void readFile() = 0;
        void setValid( bool );
        void setImageFilename( const QString& );

    private:
        class Private;
        Private* d;
    };
}

#endif
