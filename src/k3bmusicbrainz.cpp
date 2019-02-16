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

#include "k3bmusicbrainz.h"

#include <musicbrainz/mb_c.h>

#include <KProtocolManager>
#include <QDebug>
#include <QUrl>


class K3b::MusicBrainz::Private
{
public:
    musicbrainz_t mb;

    QStringList titles;
    QStringList artists;
};


K3b::MusicBrainz::MusicBrainz()
{
    d = new Private;
    d->mb = mb_New();
    mb_UseUTF8( d->mb, 1 );
}


K3b::MusicBrainz::~MusicBrainz()
{
    mb_Delete( d->mb );
    delete d;
}


int K3b::MusicBrainz::query( const QByteArray& trm )
{
    d->titles.clear();
    d->artists.clear();

    if( KProtocolManager::useProxy() ) {
        QUrl proxy( KProtocolManager::proxyFor("http") );
        mb_SetProxy( d->mb, const_cast<char*>(proxy.host().toLatin1().constData()), short(proxy.port()) );
    }

    const char* args[2];
    args[0] = trm.data();
    args[1] = 0;

    if( mb_QueryWithArgs( d->mb, (char*)MBQ_TrackInfoFromTRMId, (char**)args ) ) {

        unsigned int i = 1;
        while( mb_Select(d->mb, (char*)MBS_Rewind) && mb_Select1( d->mb, (char*)MBS_SelectTrack, i ) ) {
            QByteArray data( 256, 0 );
            mb_GetResultData( d->mb, (char*)MBE_TrackGetArtistName, data.data(), 256 );
            d->artists.append( QString::fromUtf8( data ).trimmed() );
            mb_GetResultData( d->mb, (char*)MBE_TrackGetTrackName, data.data(), 256 );
            d->titles.append( QString::fromUtf8( data ).trimmed() );

            ++i;
        }

        return i-1;
    }
    else {
        char buffer[256];
        mb_GetQueryError( d->mb, buffer, 256 );
        qDebug() << "(K3b::MusicBrainz) query error: " << buffer;
        return 0;
    }
}


QString K3b::MusicBrainz::title( unsigned int i ) const
{
    return d->titles[i];
}


QString K3b::MusicBrainz::artist( unsigned int i ) const
{
    return d->artists[i];
}
