/*

    SPDX-FileCopyrightText: 2007-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_DATA_MULTISESSION_PARAMETER_JOB_H
#define _K3B_DATA_MULTISESSION_PARAMETER_JOB_H

#include "k3bthreadjob.h"

#include "k3bdatadoc.h"


namespace K3b {
    class DataMultiSessionParameterJob : public ThreadJob
    {
        Q_OBJECT

    public:
        DataMultiSessionParameterJob( DataDoc*, JobHandler*, QObject* parent );
        ~DataMultiSessionParameterJob() override;

        DataDoc::MultiSessionMode usedMultiSessionMode() const;
        unsigned int previousSessionStart() const;
        unsigned int nextSessionStart() const;
        bool importPreviousSession() const;

    private:
        bool run() override;

        DataDoc::MultiSessionMode determineMultiSessionModeFromMedium();
        bool setupMultiSessionParameters();

        class Private;
        Private* const d;
    };
}

#endif
