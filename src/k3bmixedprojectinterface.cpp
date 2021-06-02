/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3bmixedprojectinterface.h"
#include "k3baudioprojectinterface.h"
#include "k3bdataprojectinterface.h"
#include "k3bmixeddoc.h"

namespace K3b {


MixedProjectInterface::MixedProjectInterface( MixedDoc* doc )
:
    ProjectInterface( doc ),
    m_mixedDoc( doc )
{
    m_audioInterface = new AudioProjectInterface( doc->audioDoc(), dbusPath() + "/audio" );
    m_dataInterface = new DataProjectInterface( doc->dataDoc(), dbusPath() + "/data" );
}

} // namespace K3b


