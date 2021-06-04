/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_FILE_ANALYZER_JOB_H_
#define _K3B_AUDIO_FILE_ANALYZER_JOB_H_

#include "k3bthreadjob.h"
#include "k3b_export.h"

namespace K3b {
    class AudioDecoder;

    /**
     * A simple convenience   class that runs AudioDecoder::analyseFile
     * in a different thread.
     */
    class LIBK3B_EXPORT AudioFileAnalyzerJob : public ThreadJob
    {
        Q_OBJECT

    public:
        AudioFileAnalyzerJob( JobHandler* hdl, QObject* parent );
        ~AudioFileAnalyzerJob() override;

        /**
         * Set the decoder that does the analyzation.
         */
        void setDecoder( AudioDecoder* decoder );
        AudioDecoder* decoder() const;

    private:
        bool run() override;

        class Private;
        Private* const d;
    };
}

#endif
