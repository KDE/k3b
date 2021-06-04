/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
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
