/*
 *
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 * Copyright (C) 2010 Dario Freddi <drf@kde.org>
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

#ifndef _K3BSETUPWORKER_H_
#define _K3BSETUPWORKER_H_

#include <kauth.h>

using namespace KAuth;
 
namespace K3b {
namespace Setup {
 
class Worker : public QObject
{
    Q_OBJECT
    
public:
    Worker();

public slots:
    /**
     * Updates permissions of devices and programs
     * @param burningGroup name of the burning group. If not set burning group will not be used
     * @param devices list of devices which will have updated permissions
     * @param programs list of the programs which will have updated permissions. Each element
     *                 of the list is a @see K3b::Setup::ProgramItem object
     */
    ActionReply save( QVariantMap args );

};

} // namespace Setup
} // namespace K3b
 
#endif
