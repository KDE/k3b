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

#ifndef _K3B_VIDEODVD_SUBPICTURE_STREAM_H_
#define _K3B_VIDEODVD_SUBPICTURE_STREAM_H_

#include "k3b_export.h"

#include <QString>

namespace K3b {
    namespace VideoDVD
    {
        enum SubPictureCodeMode {
            SUBPIC_CODE_MODE_RLE = 0,
            SUBPIC_CODE_MODE_EXT = 1
        };

        enum SubPictureCodeExtension {
            SUBPIC_CODE_EXT_UNSPECIFIED = 0,
            SUBPIC_CODE_EXT_CAPTION_NORMAL_SIZE = 1,
            SUBPIC_CODE_EXT_CAPTION_BIGGER_SIZE = 2,
            SUBPIC_CODE_EXT_CAPTION_FOR_CHILDREN = 3,
            SUBPIC_CODE_EXT_CLOSED_CAPTION_NORMAL_SIZE = 5,
            SUBPIC_CODE_EXT_CLOSED_CAPTION_BIGGER_SIZE = 6,
            SUBPIC_CODE_EXT_CLOSED_CAPTION_FOR_CHILDREN = 7,
            SUBPIC_CODE_EXT_FORCED_CAPTION = 9,
            SUBPIC_CODE_EXT_DIR_COMMENTS_NORMAL_SIZE = 13,
            SUBPIC_CODE_EXT_DIR_COMMENTS_BIGGER_SIZE = 14,
            SUBPIC_CODE_EXT_DIR_COMMENTS_FOR_CHILDREN = 15
        };

        class LIBK3B_EXPORT SubPictureStream
        {
        public:
            SubPictureStream() {}

            unsigned int codeMode() const { return m_codeMode; }
            unsigned int codeExtension() const { return m_codeExtension; }

            /**
             * \return A two chars language code or the empty string
             * if the language is undefined.
             */
            const QString& langCode() const { return m_langCode; }

        private:
            unsigned int m_codeMode:3;
            QString m_langCode;
            unsigned int m_codeExtension;

            friend class VideoDVD;
        };
    }
}

#endif
