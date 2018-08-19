/*
 *
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_VIDEODVD_TITLE_H_
#define _K3B_VIDEODVD_TITLE_H_

#include "k3b_export.h"

#include "k3bvideodvdtime.h"
#include "k3bvideodvdvideostream.h"
#include "k3bvideodvdaudiostream.h"
#include "k3bvideodvdsubpicturestream.h"
#include "k3bvideodvdptt.h"

#include <QVector>


namespace K3b {
    namespace VideoDVD
    {
        class LIBK3B_EXPORT Title
        {
        public:
            Title() {}

            unsigned int titleNumber() const { return m_titleNum; }

            /**
             * \return The number of PTTs (Part of Title), commonly known
             *         as chapters
             */
            unsigned int numPTTs() const { return m_numPTTs; }

            /**
             * This method is just here for convenience. It returns the same as the above.
             */
            unsigned int numChapters() const { return m_numPTTs; }
            unsigned int numAngles() const { return m_numAngles; }

            /**
             * \return The number of the titleset this title is a part of.
             */
            unsigned int titleSet() const { return m_titleSet; }

            /**
             * \return Number of the title in it's titleset.
             */
            unsigned int ttn() const { return m_ttn; }

            unsigned int numAudioStreams() const { return m_audioStreams.count(); }
            unsigned int numSubPictureStreams() const { return m_subPictureStreams.count(); }

            const VideoStream& videoStream() const { return m_videoStream; }
            const AudioStream& audioStream( unsigned int i ) const { return m_audioStreams[i]; }
            const SubPictureStream& subPictureStream( unsigned int i ) const { return m_subPictureStreams[i]; }

            /**
             * Access to the PTTs of the title
             */
            const PTT& operator[]( int i ) const { return ptt( i ); }

            /**
             * Access to the PTTs of the title
             */
            const PTT& ptt( int i ) const { return m_ptts[i]; }

            /**
             * Access to the PTTs (chapters) of the title
             */
            const PTT& chapter( int i ) const { return ptt( i ); }

            const Time& playbackTime() const { return m_playbackTime; }

            /**
             * \return A video capture
             */
            //      QBitmap videoCapture( const Time& ) const;

        private:
            unsigned int m_titleNum;
            unsigned int m_numPTTs;
            unsigned int m_titleSet;
            // FIXME: find a proper name for ttn
            unsigned int m_ttn;
            unsigned int m_numAngles;

            Time m_playbackTime;

            VideoStream m_videoStream;
            QVector<AudioStream> m_audioStreams;
            QVector<SubPictureStream> m_subPictureStreams;

            QVector<PTT> m_ptts;

            //      VideoDVD* m_videoDVD;

            friend class VideoDVD;
        };
    }
}

#endif
