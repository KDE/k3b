/*
    SPDX-FileCopyrightText: 2005-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudiocdtrackreader.h"
#include "k3baudiocdtracksource.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3bcdparanoialib.h"
#include "k3bcore.h"
#include "k3bdevice.h"
#include "k3btoc.h"
#include "k3bthreadwidget.h"
#include "k3b_i18n.h"

#include <QDebug>

#ifdef Q_OS_WIN32
#undef S_OK
#endif

namespace K3b {

class AudioCdTrackReader::Private
{
public:
    Private( AudioCdTrackSource& s )
    :
        source( s ),
        initialized( false ),
        cdParanoiaLib( 0 )
    {
    }

    AudioCdTrackSource& source;
    bool initialized;
    QScopedPointer<CdparanoiaLib> cdParanoiaLib;

    bool initParanoia();
    void closeParanoia();
};


bool AudioCdTrackReader::Private::initParanoia()
{
    if( !initialized ) {
        if( !cdParanoiaLib )
            cdParanoiaLib.reset( CdparanoiaLib::create() );

        if( cdParanoiaLib ) {
            Device::Device* device = source.searchForAudioCD();

            // ask here for the cd since searchForAudioCD() may also be called from outside
            if( !device ) {
                // could not find the CD, so ask for it
                QString s = i18n("Please insert Audio CD %1%2"
                                 ,QString::number(source.discId()),
                                 source.cdTitle().isEmpty() || source.cdArtist().isEmpty()
                                 ? QString()
                                 : " (" + source.cdArtist() + " - " + source.cdTitle() + ')');

                while( Device::Device* dev = ThreadWidget::selectDevice( source.track()->doc()->view(), s ) ) {
                    if( dev->readToc().discId() == source.discId() ) {
                        device = dev;
                        break;
                    }
                }
            }

            // user canceled
            if( !device )
                return false;

            source.setDevice( device );
            k3bcore->blockDevice( device );

            if( source.toc().isEmpty() )
                source.setToc( device->readToc() );

            if( !cdParanoiaLib->initParanoia( device, source.toc() ) ) {
                k3bcore->unblockDevice( device );
                return false;
            }

            if( source.doc() ) {
                cdParanoiaLib->setParanoiaMode( source.doc()->audioRippingParanoiaMode() );
                cdParanoiaLib->setNeverSkip( !source.doc()->audioRippingIgnoreReadErrors() );
                cdParanoiaLib->setMaxRetries( source.doc()->audioRippingRetries() );
            }

            const int start = source.toc()[source.cdTrackNumber()-1].firstSector().lba();
            cdParanoiaLib->initReading( start + source.startOffset().lba(),
                                        start + source.lastSector().lba() );

            // we only block during the initialization because we cannot determine the end of the reading process :(
            k3bcore->unblockDevice( device );

            initialized = true;
            qDebug() << "cdParanoia initialized";
        }
    }

    return initialized;
}


void AudioCdTrackReader::Private::closeParanoia()
{
    if( cdParanoiaLib && initialized ) {
        cdParanoiaLib->close();
    }
    initialized = false;
}


AudioCdTrackReader::AudioCdTrackReader( AudioCdTrackSource& source, QObject* parent )
    : QIODevice( parent ),
      d( new Private( source ) )
{
}


AudioCdTrackReader::~AudioCdTrackReader()
{
    close();
}


bool AudioCdTrackReader::open( QIODevice::OpenMode mode )
{
    if( !mode.testFlag( QIODevice::WriteOnly ) &&
        d->initParanoia() ) {
        return QIODevice::open( mode );
    }
    else {
        return false;
    }
}


void AudioCdTrackReader::close()
{
    d->closeParanoia();
    QIODevice::close();
}


qint64 AudioCdTrackReader::writeData( const char* /*data*/, qint64 /*len*/ )
{
    return -1;
}


qint64 AudioCdTrackReader::readData( char* data, qint64 /*maxlen*/ )
{
    if( d->cdParanoiaLib && d->initialized ) {
        int status = 0;
        char* buf = d->cdParanoiaLib->read( &status, 0, false /* big endian */ );
        if( status == CdparanoiaLib::S_OK ) {
            if( buf == 0 ) {
                // done
                d->closeParanoia();
                return -1;
            }
            else {
                ::memcpy( data, buf, CD_FRAMESIZE_RAW );
                return CD_FRAMESIZE_RAW;
            }
        }
        else {
            return -1;
        }
    }
    return -1;
}


bool AudioCdTrackReader::isSequential() const
{
    return false;
}


qint64 AudioCdTrackReader::size() const
{
    return d->source.length().audioBytes();
}


bool AudioCdTrackReader::seek( qint64 pos )
{
    if( d->cdParanoiaLib && d->initialized ) {
        Msf msfPos = Msf::fromAudioBytes( pos );
        const int start = d->source.toc()[d->source.cdTrackNumber()-1].firstSector().lba();
        d->cdParanoiaLib->initReading( start + d->source.startOffset().lba() + msfPos.lba(),
                                       start + d->source.lastSector().lba() );
        return QIODevice::seek( pos );
    }
    else {
        return false;
    }
}

} // namespace K3b
