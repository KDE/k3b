/*
    SPDX-FileCopyrightText: 2009-2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 2010 Dario Freddi <drf@kde.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3BHELPER_H
#define K3BHELPER_H

#include <QObject>

#include <kauth_version.h>
#if KAUTH_VERSION >= QT_VERSION_CHECK(5, 92, 0)
#include <KAuth/ActionReply>
#else
#include <KAuthActionReply>
#endif

using namespace KAuth;

namespace K3b {
 
class Helper : public QObject
{
    Q_OBJECT

public:
    Helper();

public slots:
    /**
     * Updates permissions of devices and programs
     * @param burningGroup name of the burning group. If not set burning group will not be used
     * @param devices list of devices which will have updated permissions
     * @param programs list of the programs which will have updated permissions. Each element
     *                 of the list is a @see K3b::HelperProgramItem object
     */
    ActionReply updatepermissions( QVariantMap args );

    /**
     * Adds user to a specified group
     * @param groupName name of the group
     * @param userName name of the user
     */
    ActionReply addtogroup( QVariantMap args );
};

} // namespace K3b
 
#endif // K3BHELPER_H
