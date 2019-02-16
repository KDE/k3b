/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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
#include "k3b_export.h"

#include <QScopedPointer>


namespace K3b {
    namespace Device {
        class Device;
        class Toc;
    }

    /**
     * Audio data source which reads it's data directly from an audio CD.
     *
     * Be aware that since GUI elements are not allowed in sources (other thread)
     * the source relies on the audio CD being inserted before any read operations.
     * It will search all available devices for the CD starting with the last used drive.
     */
    class LIBK3B_EXPORT AudioCdTrackSource : public AudioDataSource
    {
    public:
        /**
         * Default constructor to create a new source.
         */
        AudioCdTrackSource( const Device::Toc& toc,
                            int cdTrackNumber,
                            const QString& artist, const QString& title,
                            const QString& cdartist, const QString& cdtitle,
                            Device::Device* dev = 0 );

        /**
         * Constructor to create sources when loading from a project file without toc information
         */
        AudioCdTrackSource( unsigned int discid, const Msf& length, int cdTrackNumber,
                            const QString& artist, const QString& title,
                            const QString& cdartist, const QString& cdtitle );
        AudioCdTrackSource( const AudioCdTrackSource& );
        ~AudioCdTrackSource() override;

        unsigned int discId() const;
        int cdTrackNumber() const;

        QString artist() const;
        QString title() const;
        QString cdArtist() const;
        QString cdTitle() const;

        Msf originalLength() const override;
        QString type() const override;
        QString sourceComment() const override;
        AudioDataSource* copy() const override;
        QIODevice* createReader( QObject* parent = 0 ) override;

        /**
         * Searches for the corresponding Audio CD and returns the device in which it has
         * been found or 0 if it could not be found.
         */
        Device::Device* searchForAudioCD() const;

        /**
         * Set the device the source should start to look for the CD.
         */
        void setDevice( Device::Device* dev );

        const Device::Toc& toc() const;
        void setToc( const Device::Toc& toc );

    private:
        class Private;
        QScopedPointer<Private> d;
    };
}

#endif
