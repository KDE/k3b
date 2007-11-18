/*
 *
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DATA_MULTISESSION_PARAMETER_JOB_H
#define _K3B_DATA_MULTISESSION_PARAMETER_JOB_H

#include <k3bthreadjob.h>

#include "k3bdatadoc.h"


class K3bDataMultiSessionParameterJob : public K3bThreadJob
{
    Q_OBJECT

 public:
    K3bDataMultiSessionParameterJob( K3bDataDoc*, K3bJobHandler*, QObject* parent );
    ~K3bDataMultiSessionParameterJob();

    K3bDataDoc::MultiSessionMode usedMultiSessionMode() const;
    unsigned int previousSessionStart() const;
    unsigned int nextSessionStart() const;
    bool importPreviousSession() const;

 private:
    class WorkThread;
    WorkThread* m_thread;
};

#endif
