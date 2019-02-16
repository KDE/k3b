/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config-k3b.h>

#ifdef ENABLE_MUSICBRAINZ

#include "k3btrm.h"
#include "musicbrainz/mb_c.h"

#include <KProtocolManager>
#include <QDebug>
#include <QUrl>


class K3b::TRM::Private
{
public:
    trm_t trm;
    QByteArray sig;
    QByteArray rawSig;
};


K3b::TRM::TRM()
{
    d = new Private;
    d->trm = trm_New();
    d->rawSig.resize( 17 );
    d->sig.resize( 37 );
}


K3b::TRM::~TRM()
{
    trm_Delete( d->trm );
    delete d;
}


void K3b::TRM::start( const K3b::Msf& length )
{
    if( KProtocolManager::useProxy() ) {
        QUrl proxy( KProtocolManager::proxyFor("http") );
        trm_SetProxy( d->trm, const_cast<char*>(proxy.host().toLatin1().constData()), short(proxy.port()) );
    }

    trm_SetPCMDataInfo( d->trm, 44100, 2, 16 );
    trm_SetSongLength( d->trm, length.totalFrames()/75 );
}


bool K3b::TRM::generate( char* data, int len )
{
    return ( trm_GenerateSignature( d->trm, data, len ) == 1 );
}


bool K3b::TRM::finalize()
{
    if( trm_FinalizeSignature( d->trm, d->rawSig.data(), 0 ) == 0 ) {
        trm_ConvertSigToASCII( d->trm, d->rawSig.data(), d->sig.data() );
        return true;
    }
    else
        return false;
}


QByteArray K3b::TRM::rawSignature() const
{
    return d->rawSig;
}


QByteArray K3b::TRM::signature() const
{
    return d->sig;
}

#endif
