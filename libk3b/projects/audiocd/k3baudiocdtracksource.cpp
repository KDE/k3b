/*

    SPDX-FileCopyrightText: 2005-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3baudiocdtracksource.h"
#include "k3baudiocdtrackreader.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3bcore.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bdeviceselectiondialog.h"
#include "k3bthreadwidget.h"
#include "k3btoc.h"
#include "k3b_i18n.h"

#include <QDebug>


class K3b::AudioCdTrackSource::Private
{
public:
    Private( AudioCdTrackSource* source ) : q( source ) {}

    AudioCdTrackSource* q;
    unsigned int discId;
    Msf length;
    Device::Toc toc;
    int cdTrackNumber;

    QString artist;
    QString title;
    QString cdArtist;
    QString cdTitle;

    // ripping
    // we only save the device we last saw the CD in
    Device::Device* lastUsedDevice;
    Msf position;
    bool initialized;

    bool searchForAudioCD( Device::Device* ) const;
};


bool K3b::AudioCdTrackSource::Private::searchForAudioCD( K3b::Device::Device* dev ) const
{
    qDebug() << "(K3b::AudioCdTrackSource::searchForAudioCD(" << dev->description() << ")";
    K3b::Device::Toc toc = dev->readToc();
    return ( toc.discId() == discId );
}


K3b::AudioCdTrackSource::AudioCdTrackSource( const K3b::Device::Toc& toc, int cdTrackNumber,
                                              const QString& artist, const QString& title,
                                              const QString& cdartist, const QString& cdtitle,
                                              K3b::Device::Device* dev )
    : K3b::AudioDataSource(),
      d( new Private( this ) )
{
    d->discId = toc.discId();
    d->length = toc[cdTrackNumber-1].length();
    d->toc = toc;
    d->cdTrackNumber = cdTrackNumber;
    d->artist = artist;
    d->title = title;
    d->cdArtist = cdartist;
    d->cdTitle = cdtitle;
    d->lastUsedDevice = dev;
    d->initialized = false;
}


K3b::AudioCdTrackSource::AudioCdTrackSource( unsigned int discid, const K3b::Msf& length, int cdTrackNumber,
                                              const QString& artist, const QString& title,
                                              const QString& cdArtist, const QString& cdTitle )
    : K3b::AudioDataSource(),
      d( new Private( this ) )
{
    d->discId = discid;
    d->length = length;
    d->cdTrackNumber = cdTrackNumber;
    d->artist = artist;
    d->title = title;
    d->cdArtist = cdArtist;
    d->cdTitle = cdTitle;
    d->lastUsedDevice = 0;
    d->initialized = false;
}


K3b::AudioCdTrackSource::AudioCdTrackSource( const K3b::AudioCdTrackSource& source )
    : K3b::AudioDataSource( source ),
      d( new Private( this ) )
{
    d->discId = source.d->discId;
    d->toc = source.d->toc;
    d->cdTrackNumber = source.d->cdTrackNumber;
    d->artist = source.d->artist;
    d->title = source.d->title;
    d->cdArtist = source.d->cdArtist;
    d->cdTitle = source.d->cdTitle;
    d->lastUsedDevice = source.d->lastUsedDevice;
    d->initialized = false;
}


K3b::AudioCdTrackSource::~AudioCdTrackSource()
{
}


unsigned int K3b::AudioCdTrackSource::discId() const
{
    return d->discId;
}


int K3b::AudioCdTrackSource::cdTrackNumber() const
{
    return d->cdTrackNumber;
}


QString K3b::AudioCdTrackSource::artist() const
{
    return d->artist;
}


QString K3b::AudioCdTrackSource::title() const
{
    return d->title;
}


QString K3b::AudioCdTrackSource::cdArtist() const
{
    return d->cdArtist;
}


QString K3b::AudioCdTrackSource::cdTitle() const
{
    return d->cdTitle;
}


K3b::Device::Device* K3b::AudioCdTrackSource::searchForAudioCD() const
{
    qDebug() << "(K3b::AudioCdTrackSource::searchForAudioCD()";
    // first try the saved device
    if( d->lastUsedDevice && d->searchForAudioCD( d->lastUsedDevice ) )
        return d->lastUsedDevice;

    QList<K3b::Device::Device*> devices = k3bcore->deviceManager()->readingDevices();
    Q_FOREACH( K3b::Device::Device* dev, devices ) {
        if( d->searchForAudioCD( dev ) ) {
            return dev;
        }
    }

    qDebug() << "(K3b::AudioCdTrackSource::searchForAudioCD) failed.";

    return 0;
}


void K3b::AudioCdTrackSource::setDevice( K3b::Device::Device* dev )
{
    if( dev && dev != d->lastUsedDevice ) {
        d->lastUsedDevice = dev;
        if( d->initialized ) {
        }
    }
}


const K3b::Device::Toc& K3b::AudioCdTrackSource::toc() const
{
    return d->toc;
}


void K3b::AudioCdTrackSource::setToc( const Device::Toc& toc )
{
    d->toc = toc;
}


K3b::Msf K3b::AudioCdTrackSource::originalLength() const
{
    return d->length;
}


QString K3b::AudioCdTrackSource::type() const
{
    return i18n("CD Track");
}


QString K3b::AudioCdTrackSource::sourceComment() const
{
    return ki18n("Track %1 from Audio CD %2").subs(d->cdTrackNumber).subs(d->discId,0,16).toString();
}


K3b::AudioDataSource* K3b::AudioCdTrackSource::copy() const
{
    return new K3b::AudioCdTrackSource( *this );
}


QIODevice* K3b::AudioCdTrackSource::createReader( QObject* parent )
{
    return new AudioCdTrackReader( *this, parent );
}
