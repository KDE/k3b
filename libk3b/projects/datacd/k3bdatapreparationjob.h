/*

    SPDX-FileCopyrightText: 2006-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_DATA_PREPARATION_JOB_H_
#define _K3B_DATA_PREPARATION_JOB_H_

#include "k3bthreadjob.h"


namespace K3b {
    class DataDoc;
    class JobHandler;

    /**
     * The DataPreparationJob performs some checks on the data in a data project
     * It is used by th IsoImager.
     */
    class DataPreparationJob : public ThreadJob
    {
        Q_OBJECT

    public:
        DataPreparationJob( DataDoc* doc, JobHandler* hdl, QObject* parent );
        ~DataPreparationJob() override;

    private:
        bool run() override;

        class Private;
        Private* const d;
    };
}

#endif
