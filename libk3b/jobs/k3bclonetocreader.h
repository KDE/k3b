/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_CLONETOC_READER_H_
#define _K3B_CLONETOC_READER_H_

#include "k3bimagefilereader.h"

#include "k3bmsf.h"

#include "k3b_export.h"

namespace K3b {
    /**
     * Reads a cdrecord clone toc file and searches for the
     * corresponding image file.
     */
    class LIBK3B_EXPORT  CloneTocReader : public ImageFileReader
    {
    public:
        explicit CloneTocReader( const QString& filename = QString() );
        ~CloneTocReader() override;

        Msf imageSize() const;

    protected:
        void readFile() override;

        class Private;
        Private* d;
    };
}

#endif
