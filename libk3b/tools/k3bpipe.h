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

#ifndef _K3B_PIPE_H_
#define _K3B_PIPE_H_

#include "k3b_export.h"

namespace K3b {
    /**
     * The Pipe class represents a file descriptor pair
     * which can for example be used to connect two processes
     */
    class LIBK3B_EXPORT Pipe
    {
    public:
        /**
         * Creates a closed pipe object
         */
        Pipe();

        /**
         * The destructor takes care of closing the pipe
         */
        ~Pipe();

        /**
         * Open the pipe. This creates a file descriptor pair
         * which can be accessed using the in() and out()
         * methods.
         */
        bool open();

        int in() const { return m_fd[1]; }
        int out() const { return m_fd[0]; }

        /**
         * Calls both closeIn() and closeOut()
         */
        void close();

        void closeIn();
        void closeOut();

    private:
        int m_fd[2];
    };
}

#endif
