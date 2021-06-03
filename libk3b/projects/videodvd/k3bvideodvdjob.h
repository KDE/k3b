/*
    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_VIDEO_DVD_JOB_H_
#define _K3B_VIDEO_DVD_JOB_H_

#include "k3bdatajob.h"


namespace K3b {
    class VideoDvdDoc;
}

namespace K3b {
    /**
     * This class heavily depends on DataJob and uses some of it's internals.
     */
    class VideoDvdJob : public DataJob
    {
        Q_OBJECT

    public:
        VideoDvdJob( VideoDvdDoc*, JobHandler*, QObject* parent = 0 );
        ~VideoDvdJob() override;

        QString jobDescription() const override;
        QString jobDetails() const override;

    private:
        void prepareImager() override;

        VideoDvdDoc* m_doc;
    };
}

#endif
