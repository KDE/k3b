/*
 *
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bsetupprogramitem.h"

namespace K3b {
namespace Setup {

ProgramItem::ProgramItem()
{
}

ProgramItem::ProgramItem( const QString& path, bool needSuid )
:
    m_path( path ),
    m_needSuid( needSuid )
{
}

} // namespace Setup
} // namespace K3b

QDBusArgument& operator<<( QDBusArgument& argument, const K3b::Setup::ProgramItem& item )
{
    argument.beginStructure();
    argument << item.m_path << item.m_needSuid;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>( const QDBusArgument& argument, K3b::Setup::ProgramItem& item )
{
    argument.beginStructure();
    argument >> item.m_path >> item.m_needSuid;
    argument.endStructure();
    return argument;
}
