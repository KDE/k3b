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

#include "k3baudiocdtrackdrag.h"

#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bcore.h>

#include <qdatastream.h>


// FIXME: multiple tracks
K3b::AudioCdTrackDrag::AudioCdTrackDrag( const K3b::Device::Toc& toc, const QList<int>& cdTrackNumbers,
                                          const KCDDB::CDInfo& cddb,
                                          K3b::Device::Device* lastDev, QWidget* dragSource )
    : Q3StoredDrag( "k3b/audio_track_drag", dragSource, name ),
      m_toc(toc),
      m_cdTrackNumbers(cdTrackNumbers),
      m_cddb(cddb),
      m_device(lastDev)
{
    QByteArray data;
    QDataStream s( data, QIODevice::WriteOnly );
    s << (unsigned int)toc.count();
    for( K3b::Device::Toc::const_iterator it = toc.begin(); it != toc.end(); ++it ) {
        const K3b::Device::Track& track = *it;
        s << track.firstSector().lba() << track.lastSector().lba();
    }
    QTextStream t( s.device() );
    t << cddb.get( KCDDB::Artist ).toString() << endl
      << cddb.get( KCDDB::Title ).toString() << endl;
    for( unsigned int i = 0; i < toc.count(); ++i ) {
        t << cddb.track( i ).get( KCDDB::Artist ).toString() << endl
          << cddb.track( i ).get( KCDDB::Title ).toString() << endl;
    }

    s << (unsigned int)cdTrackNumbers.count();

    for( QList<int>::const_iterator it = cdTrackNumbers.begin();
         it != cdTrackNumbers.end(); ++it )
        s << *it;

    if( lastDev )
        t << lastDev->blockDeviceName() << endl;
    else
        t << endl;

    // TODO: the rest
    setEncodedData( data );
}


bool K3b::AudioCdTrackDrag::decode( const QMimeSource* e,
                                  K3b::Device::Toc& toc, QList<int>& trackNumbers,
                                  KCDDB::CDInfo& cddb, K3b::Device::Device** dev )
{
    QByteArray data = e->encodedData( "k3b/audio_track_drag" );

    QDataStream s( data, QIODevice::ReadOnly );

    unsigned int trackCnt;
    s >> trackCnt;
    for( unsigned int i = 0; i < trackCnt; ++i ) {
        int fs, ls;
        s >> fs;
        s >> ls;
        toc.append( K3b::Device::Track( fs, ls, K3b::Device::Track::AUDIO ) );
    }

    QTextStream t( s.device() );
    cddb.clear();
    cddb.set( KCDDB::Artist, t.readLine() );
    cddb.set( KCDDB::Title, t.readLine() );
    for( unsigned int i = 0; i < trackCnt; ++i ) {
        cddb.track( i ).set( KCDDB::Artist, t.readLine() );
        cddb.track( i ).set( KCDDB::Title, t.readLine() );
    }

    s >> trackCnt;
    trackNumbers.clear();
    for( unsigned int i = 0; i < trackCnt; ++i ) {
        int trackNumber = 0;
        s >> trackNumber;
        trackNumbers.append( trackNumber );
    }

    QString devName = t.readLine();
    if( dev && !devName.isEmpty() )
        *dev = k3bcore->deviceManager()->findDevice( devName );

    return true;
}
