/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
