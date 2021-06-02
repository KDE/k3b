/*

    SPDX-FileCopyrightText: 2009-2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 2010 Dario Freddi <drf@kde.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
