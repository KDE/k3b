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

#include "k3baudiocdtracksource.h"
#include "k3baudiotrack.h"
#include "k3baudiodoc.h"

#include <k3bthreadwidget.h>
#include <k3btoc.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bcdparanoialib.h>
#include <k3bdeviceselectiondialog.h>
#include <k3bcore.h>

#include <klocale.h>
#include <kdebug.h>
//Added by qt3to4:


K3b::AudioCdTrackSource::AudioCdTrackSource( const K3b::Device::Toc& toc, int cdTrackNumber,
                                              const QString& artist, const QString& title,
                                              const QString& cdartist, const QString& cdtitle,
                                              K3b::Device::Device* dev )
    : K3b::AudioDataSource(),
      m_discId( toc.discId() ),
      m_length( toc[cdTrackNumber-1].length() ),
      m_toc( toc ),
      m_cdTrackNumber( cdTrackNumber ),
      m_artist( artist ),
      m_title( title ),
      m_cdArtist( cdartist ),
      m_cdTitle( cdtitle ),
      m_lastUsedDevice( dev ),
      m_cdParanoiaLib( 0 ),
      m_initialized( false )
{
}


K3b::AudioCdTrackSource::AudioCdTrackSource( unsigned int discid, const K3b::Msf& length, int cdTrackNumber,
                                              const QString& artist, const QString& title,
                                              const QString& cdArtist, const QString& cdTitle )
    : K3b::AudioDataSource(),
      m_discId( discid ),
      m_length( length ),
      m_cdTrackNumber( cdTrackNumber ),
      m_artist( artist ),
      m_title( title ),
      m_cdArtist( cdArtist ),
      m_cdTitle( cdTitle ),
      m_lastUsedDevice( 0 ),
      m_cdParanoiaLib( 0 ),
      m_initialized( false )
{
}


K3b::AudioCdTrackSource::AudioCdTrackSource( const K3b::AudioCdTrackSource& source )
    : K3b::AudioDataSource( source ),
      m_discId( source.m_discId ),
      m_toc( source.m_toc ),
      m_cdTrackNumber( source.m_cdTrackNumber ),
      m_artist( source.m_artist ),
      m_title( source.m_title ),
      m_cdArtist( source.m_cdArtist ),
      m_cdTitle( source.m_cdTitle ),
      m_lastUsedDevice( source.m_lastUsedDevice ),
      m_cdParanoiaLib( 0 ),
      m_initialized( false )
{
}


K3b::AudioCdTrackSource::~AudioCdTrackSource()
{
    closeParanoia();
    delete m_cdParanoiaLib;
}


bool K3b::AudioCdTrackSource::initParanoia()
{
    if( !m_initialized ) {
        if( !m_cdParanoiaLib )
            m_cdParanoiaLib = K3b::CdparanoiaLib::create();

        if( m_cdParanoiaLib ) {
            m_lastUsedDevice = searchForAudioCD();

            // ask here for the cd since searchForAudioCD() may also be called from outside
            if( !m_lastUsedDevice ) {
                // could not find the CD, so ask for it
                QString s = i18n("Please insert Audio CD %1%2"
                                 ,QString::number(m_discId),
                                 m_cdTitle.isEmpty() || m_cdArtist.isEmpty()
                                 ? QString()
                                 : " (" + m_cdArtist + " - " + m_cdTitle + ")");

                while( K3b::Device::Device* dev = K3b::ThreadWidget::selectDevice( track()->doc()->view(), s ) ) {
                    if( searchForAudioCD( dev ) ) {
                        m_lastUsedDevice = dev;
                        break;
                    }
                }
            }

            // user canceled
            if( !m_lastUsedDevice )
                return false;

            k3bcore->blockDevice( m_lastUsedDevice );

            if( m_toc.isEmpty() )
                m_toc = m_lastUsedDevice->readToc();

            if( !m_cdParanoiaLib->initParanoia( m_lastUsedDevice, m_toc ) ) {
                k3bcore->unblockDevice( m_lastUsedDevice );
                return false;
            }

            if( doc() ) {
                m_cdParanoiaLib->setParanoiaMode( doc()->audioRippingParanoiaMode() );
                m_cdParanoiaLib->setNeverSkip( !doc()->audioRippingIgnoreReadErrors() );
                m_cdParanoiaLib->setMaxRetries( doc()->audioRippingRetries() );
            }

            m_cdParanoiaLib->initReading( m_toc[m_cdTrackNumber-1].firstSector().lba() + startOffset().lba() + m_position.lba(),
                                          m_toc[m_cdTrackNumber-1].firstSector().lba() + lastSector().lba() );

            // we only block during the initialization because we cannot determine the end of the reading process :(
            k3bcore->unblockDevice( m_lastUsedDevice );

            m_initialized = true;
            kDebug() << "(K3b::AudioCdTrackSource) initialized.";
        }
    }

    return m_initialized;
}


void K3b::AudioCdTrackSource::closeParanoia()
{
    if( m_cdParanoiaLib && m_initialized ) {
        m_cdParanoiaLib->close();
    }
    m_initialized = false;
}


K3b::Device::Device* K3b::AudioCdTrackSource::searchForAudioCD() const
{
    kDebug() << "(K3b::AudioCdTrackSource::searchForAudioCD()";
    // first try the saved device
    if( m_lastUsedDevice && searchForAudioCD( m_lastUsedDevice ) )
        return m_lastUsedDevice;

    QList<K3b::Device::Device*> devices = k3bcore->deviceManager()->readingDevices();
    Q_FOREACH( K3b::Device::Device* dev, devices ) {
        if( searchForAudioCD( dev ) ) {
            return dev;
        }
    }

    kDebug() << "(K3b::AudioCdTrackSource::searchForAudioCD) failed.";

    return 0;
}


bool K3b::AudioCdTrackSource::searchForAudioCD( K3b::Device::Device* dev ) const
{
    kDebug() << "(K3b::AudioCdTrackSource::searchForAudioCD(" << dev->description() << ")";
    K3b::Device::Toc toc = dev->readToc();
    return ( toc.discId() == m_discId );
}


void K3b::AudioCdTrackSource::setDevice( K3b::Device::Device* dev )
{
    if( dev && dev != m_lastUsedDevice ) {
        m_lastUsedDevice = dev;
        if( m_initialized ) {
        }
    }
}


K3b::Msf K3b::AudioCdTrackSource::originalLength() const
{
    return m_length;
}


bool K3b::AudioCdTrackSource::seek( const K3b::Msf& msf )
{
    // HACK: to reinitialize every time we restart the decoding
    if( msf == 0 && m_cdParanoiaLib )
        closeParanoia();

    m_position = msf;

    if( m_cdParanoiaLib )
        m_cdParanoiaLib->initReading( m_toc[m_cdTrackNumber-1].firstSector().lba() + startOffset().lba() + m_position.lba(),
                                      m_toc[m_cdTrackNumber-1].firstSector().lba() + lastSector().lba() );

    return true;
}


int K3b::AudioCdTrackSource::read( char* data, unsigned int )
{
    if( initParanoia() ) {
        int status = 0;
        char* buf = m_cdParanoiaLib->read( &status, 0, false /* big endian */ );
        if( status == K3b::CdparanoiaLib::S_OK ) {
            if( buf == 0 ) {
                // done
                closeParanoia();
                return 0;
            }
            else {
                ++m_position;
                ::memcpy( data, buf, CD_FRAMESIZE_RAW );
                return CD_FRAMESIZE_RAW;
            }
        }
        else {
            // in case the reading fails we go back to "not initialized"
            closeParanoia();
            return -1;
        }
    }
    else
        return -1;
}


QString K3b::AudioCdTrackSource::type() const
{
    return i18n("CD Track");
}


QString K3b::AudioCdTrackSource::sourceComment() const
{
    return ki18n("Track %1 from Audio CD %2").subs(m_cdTrackNumber).subs(m_discId,0,16).toString();
}


K3b::AudioDataSource* K3b::AudioCdTrackSource::copy() const
{
    return new K3b::AudioCdTrackSource( *this );
}
