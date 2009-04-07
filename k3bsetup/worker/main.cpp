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

#include "k3bsetupworker.h"
 
#include <QCoreApplication>
#include <QStringList>
 
int main( int argc, char** argv )
{
    QCoreApplication app( argc, argv );
 
    QCoreApplication::setOrganizationName( "k3b" );
    QCoreApplication::setOrganizationDomain( "k3b.org" );
    QCoreApplication::setApplicationName( "K3b Setup Worker" );
    QCoreApplication::setApplicationVersion( "0.1" );
 
    new K3b::Setup::Worker( 0 );
 
    app.exec();
}
