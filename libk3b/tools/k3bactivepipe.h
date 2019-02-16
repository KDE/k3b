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

#ifndef _K3B_ACTIVE_PIPE_H_
#define _K3B_ACTIVE_PIPE_H_

#include "k3b_export.h"

#include <QIODevice>


namespace K3b {
    /**
     * The active pipe pumps data from a source to a sink using an
     * additional thread.
     *
     * The active pumping is only performed if both the source and sink
     * QIODevices are set. Otherwise the pipe only serves as a conduit for
     * data streams. The latter is mostly interesting when using the ChecksumPipe
     * in combination with a Job that can only push data (like the DataTrackReader).
     */
    class LIBK3B_EXPORT ActivePipe : public QIODevice
    {
        Q_OBJECT

    public:
        ActivePipe();
        ~ActivePipe() override;

        /**
         * Opens the pipe and thus starts the
         * pumping.
         *
         * \param closeWhenDone If true the pipes will be closed
         *        once all data has been read.
         */
        virtual bool open( bool closeWhenDone = false );

        /**
         * Close the pipe
         */
        void close() override;

        /**
         * Read from a QIODevice instead of a file descriptor.
         * The device will be opened QIODevice::ReadOnly and closed
         * afterwards.
         *
         * \param close If true the device will be closed once close() is called.
         */
        void readFrom( QIODevice* dev, bool close = false );

        /**
         * Write to a QIODevice instead of using the readyRead signal.
         * The device will be opened QIODevice::WriteOnly and closed
         * afterwards.
         *
         * \param close If true the device will be closed once close() is called.
         */
        void writeTo( QIODevice* dev, bool close = false );

        /**
         * The number of bytes that have been read.
         */
        quint64 bytesRead() const;

        /**
         * The number of bytes that have been written.
         */
        quint64 bytesWritten() const;

    protected:
        /**
         * Reads the data from the source.
         * The default implementation reads from the file desc
         * set via readFromFd or from in()
         */
        qint64 readData( char* data, qint64 max ) override;

        /**
         * Write the data to the sink.
         * The default implementation writes to the file desc
         * set via writeToFd or out()
         *
         * Can be reimplememented to further process the data.
         */
        qint64 writeData( const char* data, qint64 max ) override;

        /**
         * Hidden open method. Use open(bool).
         */
        bool open( OpenMode mode ) override;

    private:
        class Private;
        Private* d;

        Q_PRIVATE_SLOT( d, void _k3b_close() )
    };
}

#endif
