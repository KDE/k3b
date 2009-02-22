/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_FILE_SPLITTER_H_
#define _K3B_FILE_SPLITTER_H_

#include <qiodevice.h>
#include <qstring.h>

#include <k3b_export.h>


namespace K3b {
    /**
     * QFile replacement which splits
     * big files according to the underlying file system's
     * maximum file size.
     *
     * The filename will be changed to include a counter
     * if the file has to be splitted like so:
     *
     * <pre>
     * filename.iso
     * filename.iso.001
     * filename.iso.002
     * ...
     * </pre>
     */
    class LIBK3B_EXPORT FileSplitter : public QIODevice
    {
    public:
        FileSplitter();
        FileSplitter( const QString& filename );
        ~FileSplitter();

        /**
         * Set the maximum file size. If this is set to 0
         * (the default) the max filesize is determined based on
         * the filesystem type.
         *
         * Be aware that setName will reset the max file size.
         */
        void setMaxFileSize( qint64 size );

        const QString& name() const;

        void setName( const QString& filename );

        virtual bool open( OpenMode mode );

        virtual void close();

        /**
         * File descriptor to read from and write to.
         * Not implemented yet!
         */
        int handle() const;

        virtual void flush();

        /**
         * Not implemented
         */
        virtual qint64 size() const;

        /**
         * Not implemented
         */
        virtual qint64 pos() const;

        /**
         * Not implemented
         */
        virtual bool seek( qint64 );

        virtual bool atEnd() const;

        /**
         * Deletes all the splitted files.
         * Caution: Does remove all files that fit the naming scheme without any
         * additional checks.
         */
        void remove();

    protected:
        virtual qint64 readData( char *data, qint64 maxlen );
        virtual qint64 writeData( const char *data, qint64 len );

    private:
        class Private;
        Private* d;
    };
}

#endif
