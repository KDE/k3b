/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_MOVIX_JOB_H_
#define _K3B_MOVIX_JOB_H_

#include "k3bjob.h"

namespace K3b {
    class MovixDoc;
    class DataJob;
    class MovixDocPreparer;
    class Doc;

    namespace Device {
        class Device;
    }

    class MovixJob : public BurnJob
    {
        Q_OBJECT

    public:
        MovixJob( MovixDoc* doc, JobHandler*, QObject* parent = 0 );
        ~MovixJob() override;

        Doc* doc() const;
        Device::Device* writer() const override;

        QString jobDescription() const override;
        QString jobDetails() const override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

    private Q_SLOTS:
        void slotDataJobFinished( bool );

    private:
        MovixDoc* m_doc;
        DataJob* m_dataJob;
        MovixDocPreparer* m_movixDocPreparer;

        bool m_canceled;
    };
}

#endif
