/*
    SPDX-FileCopyrightText: 2005-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudiocdtrackdrag.h"

#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3btoc.h"
#include "k3btrack.h"
#include "k3bcore.h"

#include <QDataStream>
#include <QMimeData>


// FIXME: multiple tracks
K3b::AudioCdTrackDrag::AudioCdTrackDrag()
    : m_device( 0 )
{
}


K3b::AudioCdTrackDrag::AudioCdTrackDrag( const K3b::Device::Toc& toc,
                                         const QList<int>& trackNumbers,
                                         const KCDDB::CDInfo& cddb,
                                         K3b::Device::Device* lastDev )
    : m_toc(toc),
      m_trackNumbers(trackNumbers),
      m_cddb(cddb),
      m_device(lastDev)
{
}


void K3b::AudioCdTrackDrag::populateMimeData( QMimeData* mime )
{
    QByteArray data;
    QDataStream s( &data, QIODevice::WriteOnly );

    // encode TOC
    s << m_toc.count();
    for( K3b::Device::Toc::const_iterator it = m_toc.constBegin(); it != m_toc.constEnd(); ++it ) {
        const K3b::Device::Track& track = *it;
        s << track.firstSector().lba() << track.lastSector().lba();
    }

    // encode CDDB
    s << m_cddb.get( KCDDB::Artist ).toString()
      << m_cddb.get( KCDDB::Title ).toString();
    for( int i = 0; i < m_toc.count(); ++i ) {
        s << m_cddb.track( i ).get( KCDDB::Artist ).toString()
          << m_cddb.track( i ).get( KCDDB::Title ).toString();
    }

    // encode dragged track numbers
    s << m_trackNumbers.count();
    foreach( int trackNumber, m_trackNumbers )
        s << trackNumber;

    // encode last used device
    if( m_device )
        s << m_device->blockDeviceName();
    else
        s << QString();

    // TODO: the rest
    mime->setData( mimeDataTypes().first(), data );
}


K3b::AudioCdTrackDrag K3b::AudioCdTrackDrag::fromMimeData( const QMimeData* mime )
{
    AudioCdTrackDrag drag;

    QByteArray data = mime->data( mimeDataTypes().first() );

    QDataStream s( data );

    // decode TOC
    int trackCnt;
    s >> trackCnt;
    for( int i = 0; i < trackCnt; ++i ) {
        int fs, ls;
        s >> fs;
        s >> ls;
        drag.m_toc.append( K3b::Device::Track( fs, ls, K3b::Device::Track::TYPE_AUDIO ) );
        qDebug() << "decoded track:" << drag.m_toc.last();
    }

    // decode cddb
    drag.m_cddb.clear();
    QString str;
    s >> str;
    drag.m_cddb.set( KCDDB::Artist, str );
    s >> str;
    drag.m_cddb.set( KCDDB::Title, str );
    for( int i = 0; i < trackCnt; ++i ) {
        s >> str;
        qDebug() << "Decoded artist for track" << ( i+1 ) << str;
        drag.m_cddb.track( i ).set( KCDDB::Artist, str );
        s >> str;
        qDebug() << "Decoded title for track" << ( i+1 ) << str;
        drag.m_cddb.track( i ).set( KCDDB::Title, str );
    }

    // decode track numbers
    s >> trackCnt;
    drag.m_trackNumbers.clear();
    for( int i = 0; i < trackCnt; ++i ) {
        int track = 0;
        s >> track;
        drag.m_trackNumbers.append( track );
    }

    // decoce last used device
    s >> str;
    if( !str.isEmpty() )
        drag.m_device = k3bcore->deviceManager()->findDevice( str );

    return drag;
}


QStringList K3b::AudioCdTrackDrag::mimeDataTypes()
{
    return QStringList() << QLatin1String( "k3b/audio_track_list" );
}


bool K3b::AudioCdTrackDrag::canDecode( const QMimeData* s )
{
    return s->hasFormat( mimeDataTypes().first() );
}
