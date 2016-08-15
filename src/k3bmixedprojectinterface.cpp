/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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


