/*
 *
 * Copyright (C) 2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_RAW_AUDIO_DATA_SOURCE_H_
#define _K3B_RAW_AUDIO_DATA_SOURCE_H_

#include "k3baudiodatasource.h"
#include "k3b_export.h"

namespace K3b {
    /**
     * Data source reading raw, unencoded audio CD data, ie. raw audio cd
     * blocks/sectors.
     *
     * This source is mostly useful for cue/bin images.
     */
    class LIBK3B_EXPORT RawAudioDataSource : public AudioDataSource
    {
    public:
        RawAudioDataSource();
        explicit RawAudioDataSource( const QString& path );
        RawAudioDataSource( const RawAudioDataSource& );
        ~RawAudioDataSource() override;

        QString path() const;

        Msf originalLength() const override;

        QString type() const override;
        QString sourceComment() const override;

        AudioDataSource* copy() const override;
        QIODevice* createReader( QObject* parent = 0 ) override;

    private:
        class Private;
        Private* const d;
    };
}

#endif
