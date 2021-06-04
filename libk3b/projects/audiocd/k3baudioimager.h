/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_IMAGER_H_
#define _K3B_AUDIO_IMAGER_H_

#include "k3bthreadjob.h"

class QIODevice;

namespace K3b {
    class AudioDoc;
    class AudioJobTempData;

    class AudioImager : public ThreadJob
    {
        Q_OBJECT

    public:
        AudioImager( AudioDoc* doc, AudioJobTempData* tempData, JobHandler* jh, QObject* parent = 0 );
        ~AudioImager() override;

        /**
         * The data gets written directly into fd instead of the imagefiles.
         * Be aware that this only makes sense before starting the job.
         * To disable just set dev to 0
         */
        void writeTo( QIODevice* dev );

        enum ErrorType {
            ERROR_FD_WRITE,
            ERROR_DECODING_TRACK,
            ERROR_UNKNOWN
        };

        ErrorType lastErrorType() const;

    private:
        bool run() override;

        class Private;
        Private* const d;
    };
}

#endif
