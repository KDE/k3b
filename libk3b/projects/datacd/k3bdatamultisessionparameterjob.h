/*
 *
 * Copyright (C) 2007-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DATA_MULTISESSION_PARAMETER_JOB_H
#define _K3B_DATA_MULTISESSION_PARAMETER_JOB_H

#include <k3bthreadjob.h>

#include "k3bdatadoc.h"


namespace K3b {
    class DataMultiSessionParameterJob : public ThreadJob
    {
        Q_OBJECT

    public:
        DataMultiSessionParameterJob( DataDoc*, JobHandler*, QObject* parent );
        ~DataMultiSessionParameterJob();

        DataDoc::MultiSessionMode usedMultiSessionMode() const;
        unsigned int previousSessionStart() const;
        unsigned int nextSessionStart() const;
        bool importPreviousSession() const;

    private:
        bool run();

        DataDoc::MultiSessionMode determineMultiSessionModeFromMedium();
        bool setupMultiSessionParameters();

        class Private;
        Private* const d;
    };
}

#endif
