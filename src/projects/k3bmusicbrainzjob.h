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

#ifndef _K3B_MUSICBRAINZ_JOB_H_
#define _K3B_MUSICBRAINZ_JOB_H_

#include "k3bjob.h"


namespace K3b {
    class AudioTrack;
}
class QWidget;


/**
 * This job tries to determine AudioTrack's title and artist using
 * Musicbrainz.
 */
namespace K3b {
    class MusicBrainzJob : public Job
    {
        Q_OBJECT

    public:
        /**
         * \param parent since we do not use this job with a normal progressdialog we need a widget
         *        as parent
         */
        explicit MusicBrainzJob( QWidget* parent = 0 );
        ~MusicBrainzJob() override;

        bool hasBeenCanceled() const override;

    Q_SIGNALS:
        /**
         * Emitted for each track. This is signal can be used
         * to display further information.
         *
         * \param track The track for which metadata was searched.
         * \param success True if metadata was found
         */
        void trackFinished( K3b::AudioTrack* track, bool success );

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        void setTracks( const QList<K3b::AudioTrack*>& tracks );

    private Q_SLOTS:
        void slotTrmPercent( int p );
        void slotMbJobFinished( bool success );

    private:
        class Private;
        Private* const d;
    };
}

#endif
