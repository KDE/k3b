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

#ifndef _K3B_CHECKSUM_PIPE_H_
#define _K3B_CHECKSUM_PIPE_H_

#include "k3bactivepipe.h"

#include "k3b_export.h"


namespace K3b {
    /**
     * The checksum pipe calculates the checksum of the data
     * passed through it.
     */
    class LIBK3B_EXPORT ChecksumPipe : public ActivePipe
    {
        Q_OBJECT

    public:
        ChecksumPipe();
        ~ChecksumPipe() override;

        enum Type {
            MD5
        };

        /**
         * \reimplemented
         * Defaults to MD5 checksum
         */
        bool open( bool closeWhenDone = false ) override;

        /**
         * Opens the pipe and thus starts the
         * checksum calculation
         *
         * \param closeWhenDone If true the pipes will be closed
         *        once all data has been read.
         */
        bool open( Type type, bool closeWhenDone = false );

        /**
         * Get the calculated checksum
         */
        QByteArray checksum() const;

    protected:
        qint64 writeData( const char* data, qint64 max ) override;

    private:
        /**
         * Hidden open method. Use open(bool).
         */
        bool open( OpenMode mode ) override;

        class Private;
        Private* d;
    };
}

#endif
