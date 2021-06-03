/*
    SPDX-FileCopyrightText: 2004-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "k3bprojectplugin.h"
#include "k3b_i18n.h"

K3b::ProjectPlugin::ProjectPlugin( Type type, bool gui, QObject* parent )
  : K3b::Plugin( parent ),
    m_type(type),
    m_hasGUI(gui)
{
}


QString K3b::ProjectPlugin::categoryName() const
{
    return i18nc( "plugin type", "Project plugin" );
}
