/*
 *
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@k3b.org>
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
        Q_OBJECT

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

        QString name() const;

        void setName( const QString& filename );

        virtual bool open( OpenMode mode );

        virtual void close();

        virtual void flush();

        virtual qint64 size() const;

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

        /**
         * \return \p true if the file is open in writable mode
         */
        bool waitForBytesWritten( int msecs );

        /**
         * \return \p true if open and not at end
         */
        bool waitForReadyRead( int msecs );

    protected:
        virtual qint64 readData( char *data, qint64 maxlen );
        virtual qint64 writeData( const char *data, qint64 len );

    private:
        class Private;
        Private* d;
    };
}

#endif
