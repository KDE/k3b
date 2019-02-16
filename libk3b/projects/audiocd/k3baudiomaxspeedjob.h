/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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
