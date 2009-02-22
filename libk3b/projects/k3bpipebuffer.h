/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_PIPE_BUFFER_H_
#define _K3B_PIPE_BUFFER_H_


#include <k3bthreadjob.h>

namespace K3b {
    /**
     * the pipebuffer uses the signal percent to show it's status.
     */
    class PipeBuffer : public ThreadJob
    {
        Q_OBJECT

    public:
        PipeBuffer( JobHandler*, QObject* parent = 0 );
        ~PipeBuffer();

        /**
         * Set the buffer size in MB. The default value is 4 MB.
         */
        void setBufferSize( int );

        /**
         * If this is set to -1 (which is the default) the pipebuffer
         * will create a fd pair which can be obtained by inFd() after
         * the buffer has been started.
         */
        void readFromFd( int fd );
        void writeToFd( int fd );

        /**
         * This is only valid after the piepbuffer has been started and no fd
         * has been set with readFromFd.
         */
        int inFd() const;

    public Q_SLOTS:
        void start();

    private:
        bool run();

        class Private;
        Private* const d;
    };
}

#endif
