/*
 *
 * Copyright (C) 2009-2011 Michal Malek <michalm@jabster.pl>
 * Copyright (C)      2010 Dario Freddi <drf@kde.org>
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

#include "k3bhelperprogramitem.h"

#include <QDataStream>

namespace K3b {

HelperProgramItem::HelperProgramItem()
{
}

HelperProgramItem::HelperProgramItem( const QString& path, bool needSuid )
:
    m_path( path ),
    m_needSuid( needSuid )
{
}

} // namespace K3b

QDataStream& operator<<( QDataStream& data, const K3b::HelperProgramItem& item )
{
    data << item.m_path << item.m_needSuid;
    return data;
}

const QDataStream& operator>>( QDataStream& data, K3b::HelperProgramItem& item )
{
    data >> item.m_path >> item.m_needSuid;
    return data;
}
