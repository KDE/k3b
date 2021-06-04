/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudioprojectinterface.h"
#include "k3baudioprojectinterfaceadaptor.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"

namespace K3b {

AudioProjectInterface::AudioProjectInterface( AudioDoc* doc, const QString& dbusPath )
:
    ProjectInterface( doc, dbusPath ),
    m_audioDoc( doc )
{
    new K3bAudioProjectInterfaceAdaptor( this );
}


int AudioProjectInterface::trackCount() const
{
    return m_audioDoc->numOfTracks();
}


QString AudioProjectInterface::title() const
{
    return m_audioDoc->title();
}


QString AudioProjectInterface::artist() const
{
    return m_audioDoc->artist();
}


QString AudioProjectInterface::trackTitle( int trackNum ) const
{
    AudioTrack* track = m_audioDoc->getTrack( trackNum );
    if( track )
        return track->title();
    else
        return QString();
}


QString AudioProjectInterface::trackArtist( int trackNum ) const
{
    AudioTrack* track = m_audioDoc->getTrack( trackNum );
    if( track )
        return track->artist();
    else
        return QString();
}


void AudioProjectInterface::setTitle( const QString& title )
{
    m_audioDoc->setTitle( title );
}


void AudioProjectInterface::setArtist( const QString& artist )
{
    m_audioDoc->setArtist( artist );
}


void AudioProjectInterface::setTrackTitle( int trackNum, const QString& title )
{
    AudioTrack* track = m_audioDoc->getTrack( trackNum );
    if( track )
        track->setTitle( title );
}


void AudioProjectInterface::setTrackArtist( int trackNum, const QString& artist )
{
    AudioTrack* track = m_audioDoc->getTrack( trackNum );
    if( track )
        track->setArtist( artist );
}

} // namespace K3b


