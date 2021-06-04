/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_MAX_SPEED_JOB_H_
#define _K3B_AUDIO_MAX_SPEED_JOB_H_

#include "k3bthreadjob.h"

namespace K3b {
    class AudioDoc;

    class AudioMaxSpeedJob : public ThreadJob
    {
        Q_OBJECT

    public:
        AudioMaxSpeedJob( AudioDoc* doc, JobHandler*, QObject* parent = 0 );
        ~AudioMaxSpeedJob() override;

        /**
         * KB/sec
         * Only valid if the job finished successfully.
         */
        int maxSpeed() const;

    private:
        bool run() override;

        class Private;
        Private* const d;
    };
}

#endif
