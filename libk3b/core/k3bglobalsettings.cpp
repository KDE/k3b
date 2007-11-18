/*
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bglobalsettings.h"

#include <kconfig.h>
#include <kconfiggroup.h>


K3bGlobalSettings::K3bGlobalSettings()
    : m_eject(true),
      m_burnfree(true),
      m_overburn(false),
      m_useManualBufferSize(false),
      m_bufferSize(4),
      m_force(false)
{
}


void K3bGlobalSettings::readSettings( KConfig* c )
{
    KConfigGroup globalSettings = c->group( "General Options" );

    m_eject = !globalSettings.readEntry( "No cd eject", false );
    m_burnfree = globalSettings.readEntry( "burnfree", true );
    m_overburn = globalSettings.readEntry( "Allow overburning", false );
    m_useManualBufferSize = globalSettings.readEntry( "Manual buffer size", false );
    m_bufferSize = globalSettings.readEntry( "Fifo buffer", 4 );
    m_force = globalSettings.readEntry( "Force unsafe operations", false );
}


void K3bGlobalSettings::saveSettings( KConfig* c )
{
    KConfigGroup globalSettings = c->group( "General Options" );

    globalSettings.writeEntry( "No cd eject", !m_eject );
    globalSettings.writeEntry( "burnfree", m_burnfree );
    globalSettings.writeEntry( "Allow overburning", m_overburn );
    globalSettings.writeEntry( "Manual buffer size", m_useManualBufferSize );
    globalSettings.writeEntry( "Fifo buffer", m_bufferSize );
    globalSettings.writeEntry( "Force unsafe operations", m_force );
}
