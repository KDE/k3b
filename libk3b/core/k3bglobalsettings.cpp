/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
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
#include <kstandarddirs.h>


K3bGlobalSettings::K3bGlobalSettings()
    : m_eject(true),
      m_burnfree(true),
      m_overburn(false),
      m_useManualBufferSize(false),
      m_bufferSize(4),
      m_force(false)
{
}


void K3bGlobalSettings::readSettings( const KConfigGroup& c )
{
    m_eject = !c.readEntry( "No cd eject", false );
    m_burnfree = c.readEntry( "burnfree", true );
    m_overburn = c.readEntry( "Allow overburning", false );
    m_useManualBufferSize = c.readEntry( "Manual buffer size", false );
    m_bufferSize = c.readEntry( "Fifo buffer", 4 );
    m_force = c.readEntry( "Force unsafe operations", false );
    m_defaultTempPath = c.readPathEntry( "Temp Dir", KGlobal::dirs()->resourceDirs( "tmp" ).first() );
}


void K3bGlobalSettings::saveSettings( KConfigGroup c )
{
    c.writeEntry( "No cd eject", !m_eject );
    c.writeEntry( "burnfree", m_burnfree );
    c.writeEntry( "Allow overburning", m_overburn );
    c.writeEntry( "Manual buffer size", m_useManualBufferSize );
    c.writeEntry( "Fifo buffer", m_bufferSize );
    c.writeEntry( "Force unsafe operations", m_force );
    c.writeEntry( "Temp Dir", m_defaultTempPath );
}
