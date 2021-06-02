/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef _K3B_MIXED_PROJECT_INTERFACE_H_
#define _K3B_MIXED_PROJECT_INTERFACE_H_

#include "k3bprojectinterface.h"

namespace K3b {
    class AudioProjectInterface;
    class DataProjectInterface;
    class MixedDoc;

    class MixedProjectInterface : public ProjectInterface
    {
        Q_OBJECT

    public:
        explicit MixedProjectInterface( MixedDoc* doc );

    private:
        MixedDoc* m_mixedDoc;

        AudioProjectInterface* m_audioInterface;
        DataProjectInterface* m_dataInterface;
    };
}

#endif
