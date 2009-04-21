/*
 *
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_VIDEODVD_VIDEO_STREAM_H_
#define _K3B_VIDEODVD_VIDEO_STREAM_H_

#include "k3b_export.h"

namespace K3b {
    namespace VideoDVD
    {
        enum VideoMPEGVersion {
            MPEG1 = 0,
            MPEG2 = 1
        };

        enum VideoFormat {
            VIDEO_FORMAT_NTSC = 0,
            VIDEO_FORMAT_PAL  = 1
        };

        enum VideoAspectRatio {
            VIDEO_ASPECT_RATIO_4_3  = 0,
            VIDEO_ASPECT_RATIO_16_9 = 1
        };

        enum VideoPermitedDf {
            VIDEO_PERMITTED_DF_BOTH        = 0,
            VIDEO_PERMITTED_DF_PAN_SCAN    = 1,
            VIDEO_PERMITTED_DF_LETTERBOXED = 2,
            VIDEO_PERMITTED_DF_UNSPECIFIED = 3
        };

        enum VideoBitRate {
            VIDEO_BITRATE_VARIABLE = 0,
            VIDEO_BITRATE_CONSTANT = 1
        };

        enum VideoPicureSize {
            VIDEO_PICTURE_SIZE_720   = 0,
            VIDEO_PICTURE_SIZE_704   = 1,
            VIDEO_PICTURE_SIZE_352   = 2,
            VIDEO_PICTURE_SIZE_352_2 = 3
        };

        class LIBK3B_EXPORT VideoStream
        {
        public:
            VideoStream() {}

            unsigned int permittedDf() const { return m_permittedDf; }
            unsigned int displayAspectRatio() const { return m_displayAspectRatio; }
            unsigned int format() const { return m_videoFormat; }
            unsigned int mpegVersion() const { return m_mpegVersion; }
            unsigned int filmMode() const { return m_filmMode; }
            unsigned int letterboxed() const { return m_letterboxed; }
            unsigned int pictureSize() const { return m_pictureSize; }
            unsigned int bitRate() const { return m_bitRate; }

            /**
             * The picture width of the video stream
             */
            unsigned int pictureWidth() const;

            /**
             * The picture height of the video stream
             */
            unsigned int pictureHeight() const;

            /**
             * The width of the "real" video after applying aspect ratio
             * correction
             */
            unsigned int realPictureWidth() const;

            /**
             * The height of the "real" video after applying aspect ratio
             * correction
             */
            unsigned int realPictureHeight() const;

        private:
            unsigned int m_permittedDf:2;
            unsigned int m_displayAspectRatio:2;
            unsigned int m_videoFormat:2;
            unsigned int m_mpegVersion:2;
            unsigned int m_filmMode:1;
            unsigned int m_letterboxed:1;
            unsigned int m_pictureSize:2;
            unsigned int m_bitRate:1;

            friend class VideoDVD;
        };
    }
}

#endif
