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

#include <k3b_export.h>


class QIODevice;


namespace K3b {
    /**
     * The active pipe pumps data from a source to a sink using an
     * additional thread.
     */
    class LIBK3B_EXPORT ActivePipe
    {
    public:
        ActivePipe();
        virtual ~ActivePipe();

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
        virtual void close();

        /**
         * Set the file descriptor to write to. If this is -1 (the default) then
         * data has to read from the out() file descriptor.
         *
         * \param fd The file descriptor to write to.
         * \param close If true the reading file descriptor will be closed on a call to close()
         */
        void writeToFd( int fd, bool close = false );

        /**
         * Read from a QIODevice instead of a file descriptor.
         * The device will be opened QIODevice::ReadOnly and closed
         * afterwards.
         */
        void readFromIODevice( QIODevice* dev );

        /**
         * Write to a QIODevice instead of a file descriptor.
         * The device will be opened QIODevice::WriteOnly and closed
         * afterwards.
         */
        void writeToIODevice( QIODevice* dev );

        /**
         * The file descriptor to write into
         * Only valid if no source has been set
         */
        int in() const;

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
        virtual int read( char* data, int max );

        /**
         * Write the data to the sink.
         * The default implementation writes to the file desc
         * set via writeToFd or out()
         *
         * Can be reimplememented to further process the data.
         */
        virtual int write( char* data, int max );

    private:
        class Private;
        Private* d;
    };
}

#endif
