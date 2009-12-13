/*
 *
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bsimplejobhandler.h"


K3b::SimpleJobHandler::SimpleJobHandler( QObject* parent )
    : QObject( parent ),
      K3b::JobHandler()
{
}

K3b::SimpleJobHandler::~SimpleJobHandler()
{
}

K3b::Device::MediaType K3b::SimpleJobHandler::waitForMedia( K3b::Device::Device* dev,
                                                            Device::MediaStates mediaState,
                                                            Device::MediaTypes mediaType,
                                                            const QString& message )
{
    Q_UNUSED( dev );
    Q_UNUSED( mediaState );
    Q_UNUSED( mediaType );
    Q_UNUSED( message );

    return Device::MEDIA_UNKNOWN;
}

bool K3b::SimpleJobHandler::questionYesNo( const QString& text,
                                           const QString& caption,
                                           const KGuiItem& buttonYes,
                                           const KGuiItem& buttonNo )
{
    Q_UNUSED( text );
    Q_UNUSED( caption );
    Q_UNUSED( buttonYes );
    Q_UNUSED( buttonNo );

    return true;
}

void K3b::SimpleJobHandler::blockingInformation( const QString& text,
                                                 const QString& caption )
{
    Q_UNUSED( text );
    Q_UNUSED( caption );
}

#include "k3bsimplejobhandler.moc"
