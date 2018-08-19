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

#ifndef _K3B_VIDEODVD_AUDIO_STREAM_H_
#define _K3B_VIDEODVD_AUDIO_STREAM_H_

#include "k3b_export.h"

#include <QString>


namespace K3b {
    namespace VideoDVD
    {
        enum AudioFormat {
            AUDIO_FORMAT_AC3      = 0,
            AUDIO_FORMAT_MPEG1    = 2,
            AUDIO_FORMAT_MPEG2EXT = 3,
            AUDIO_FORMAT_LPCM     = 4,
            AUDIO_FORMAT_DTS      = 6
        };

        enum AudioApplicationMode {
            AUDIO_APPLICATION_KARAOKE  = 1,
            AUDIO_APPLICATION_SURROUND = 2
        };

        enum AudioQuantization {
            AUDIO_QUANTIZATION_16BIT = 0,
            AUDIO_QUANTIZATION_20BIT = 1,
            AUDIO_QUANTIZATION_24BIT = 2
        };

        enum AudioSampleFrequency {
            AUDIO_SAMPLE_FREQ_48HZ = 0,
            AUDIO_SAMPLE_FREQ_96HZ = 1
        };

        enum AudioCodeExtension {
            AUDIO_CODE_EXT_UNSPECIFIED       = 0,
            AUDIO_CODE_EXT_NORMAL            = 1,
            AUDIO_CODE_EXT_VISUALLY_IMPAIRED = 2,
            AUDIO_CODE_EXT_DIR_COMMENTS_1    = 3,
            AUDIO_CODE_EXT_DIR_COMMENTS_2    = 4
        };

        class LIBK3B_EXPORT AudioStream
        {
        public:
            AudioStream() {}

            /**
             * \return A two chars language code or the empty string
             * if the language is undefined.
             */
            const QString& langCode() const { return m_langCode; }

            /**
             * \see AudioFormat
             */
            unsigned short format() const { return m_format; }

            /**
             * \see AudioApplicationMode
             */
            unsigned short applicationMode() const { return m_applicationMode; }

            /**
             * \see AudioQuantization
             */
            unsigned short quantization() const { return m_quantization; }

            /**
             * \see AudioSampleFrequency
             */
            unsigned short sampleFrequency() const { return m_sampleFrequency; }

            /**
             * \see AudioCodeExtension
             */
            unsigned short codeExtension() const { return m_codeExtension; }

            bool multiChannelExt() const { return m_multiChannelExt; }

            unsigned short channels() const { return m_channels; }

        private:
            unsigned short m_format:3;
            unsigned short m_applicationMode:2;
            unsigned short m_quantization:2;
            unsigned short m_sampleFrequency:2;
            unsigned short m_codeExtension;
            bool m_multiChannelExt;
            unsigned short m_channels:3;
            QString m_langCode;

            friend class VideoDVD;
        };
    }
}

#endif
