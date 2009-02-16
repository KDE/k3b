/* 
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_CD_TRACK_SOURCE_H_
#define _K3B_AUDIO_CD_TRACK_SOURCE_H_

#include "k3baudiodatasource.h"

#include <k3btoc.h>

#include "k3b_export.h"


namespace K3bDevice {
    class Device;
}
class K3bCdparanoiaLib;


#ifdef __GNUC__
#warning FIXME: move K3bMedium into libk3b, then add KCddb info to K3bMedium, and use K3bMedium for the K3bAudioCdTrackSource
#endif
/**
 * Audio data source which reads it's data directly from an audio CD.
 *
 * Be aware that since GUI elements are not allowed in sources (other thread)
 * the source relies on the audio CD being inserted before any read operations.
 * It will search all available devices for the CD starting with the last used drive.
 */
class LIBK3B_EXPORT K3bAudioCdTrackSource : public K3bAudioDataSource
{
public:
    /**
     * Default constructor to create a new source.
     */
    K3bAudioCdTrackSource( const K3bDevice::Toc& toc, 
                           int cdTrackNumber,
                           const QString& artist, const QString& title,
                           const QString& cdartist, const QString& cdtitle, 
                           K3bDevice::Device* dev = 0 );

    /**
     * Constructor to create sources when loading from a project file without toc information
     */
    K3bAudioCdTrackSource( unsigned int discid, const K3b::Msf& length, int cdTrackNumber, 
                           const QString& artist, const QString& title,
                           const QString& cdartist, const QString& cdtitle );
    K3bAudioCdTrackSource( const K3bAudioCdTrackSource& );
    ~K3bAudioCdTrackSource();

    unsigned int discId() const { return m_discId; }
    int cdTrackNumber() const { return m_cdTrackNumber; }

    QString artist() const { return m_artist; }
    QString title() const { return m_title; }
    QString cdArtist() const { return m_cdArtist; }
    QString cdTitle() const { return m_cdTitle; }

    K3b::Msf originalLength() const;
    bool seek( const K3b::Msf& );
    int read( char* data, unsigned int max );
    QString type() const;
    QString sourceComment() const;
    K3bAudioDataSource* copy() const;

    /**
     * Searches for the corresponding Audio CD and returns the device in which it has
     * been found or 0 if it could not be found.
     */
    K3bDevice::Device* searchForAudioCD() const;

    /**
     * Set the device the source should start to look for the CD.
     */
    void setDevice( K3bDevice::Device* dev );

private:
    bool initParanoia();
    void closeParanoia();
    bool searchForAudioCD( K3bDevice::Device* ) const;

    unsigned int m_discId;
    K3b::Msf m_length;
    K3bDevice::Toc m_toc;
    int m_cdTrackNumber;

    QString m_artist;
    QString m_title;
    QString m_cdArtist;
    QString m_cdTitle;

    // ripping
    // we only save the device we last saw the CD in
    K3bDevice::Device* m_lastUsedDevice;
    K3bCdparanoiaLib* m_cdParanoiaLib;
    K3b::Msf m_position;
    bool m_initialized;
};

#endif
