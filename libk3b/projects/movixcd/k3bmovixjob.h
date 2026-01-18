/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
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
        MovixJob( MovixDoc* doc, JobHandler*, QObject* parent = nullptr );
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
