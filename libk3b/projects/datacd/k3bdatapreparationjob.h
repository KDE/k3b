/* 
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DATA_PREPARATION_JOB_H_
#define _K3B_DATA_PREPARATION_JOB_H_

#include <k3bthreadjob.h>


class K3bDataDoc;
class K3bJobHandler;

/**
 * The K3bDataPreparationJob performs some checks on the data in a data project
 * It is used by th K3bIsoImager.
 */
class K3bDataPreparationJob : public K3bThreadJob
{
    Q_OBJECT

public:
    K3bDataPreparationJob( K3bDataDoc* doc, K3bJobHandler* hdl, QObject* parent );
    ~K3bDataPreparationJob();

private:
    bool run();

    class Private;
    Private* const d;
};

#endif
