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

#ifndef _K3B_CLONETOC_FILE_PARSER_H_
#define _K3B_CLONETOC_FILE_PARSER_H_

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
        CloneTocReader( const QString& filename = QString() );
        ~CloneTocReader();

        Msf imageSize() const;

    protected:
        void readFile();

        class Private;
        Private* d;
    };
}

#endif
