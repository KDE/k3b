/*

    SPDX-FileCopyrightText: 2009-2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 2010 Dario Freddi <drf@kde.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3BHELPERPROGRAMITEM_H
#define K3BHELPERPROGRAMITEM_H

#include <QMetaType>
#include <QString>

namespace K3b {

class HelperProgramItem
{
public:
    HelperProgramItem();
    HelperProgramItem( const QString& path, bool needSuid );

    QString m_path;
    bool m_needSuid;
};

} // namespace K3b

Q_DECLARE_METATYPE( K3b::HelperProgramItem )

QDataStream& operator<<( QDataStream& stream, const K3b::HelperProgramItem& item );
const QDataStream& operator>>( QDataStream& stream, K3b::HelperProgramItem& item );

#endif // K3BHELPERPROGRAMITEM_H
