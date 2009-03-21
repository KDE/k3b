/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_IMAGER_H_
#define _K3B_AUDIO_IMAGER_H_

#include <k3bthreadjob.h>

class QIODevice;

namespace K3b {
    class AudioDoc;

    class AudioImager : public ThreadJob
    {
        Q_OBJECT

    public:
        AudioImager( AudioDoc*, JobHandler*, QObject* parent = 0 );
        ~AudioImager();

        /**
         * the data gets written directly into fd instead of the imagefile.
         * Be aware that this only makes sense before starting the job.
         * To disable just set dev to 0
         */
        void writeTo( QIODevice* dev );

        /**
         * Image path. Should be an empty directory or a non-existing
         * directory in which case it will be created.
         */
        void setImageFilenames( const QStringList& p );

        enum ErrorType {
            ERROR_FD_WRITE,
            ERROR_DECODING_TRACK,
            ERROR_UNKNOWN
        };

        ErrorType lastErrorType() const;

    private:
        bool run();

        class Private;
        Private* const d;
    };
}

#endif
