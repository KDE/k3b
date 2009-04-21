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

#ifndef _K3B_VIDEODVD_PTT_H_
#define _K3B_VIDEODVD_PTT_H_

#include "k3b_export.h"

#include "k3bvideodvd.h"
#include "k3bvideodvdtime.h"

namespace K3b {
    namespace VideoDVD
    {
        class LIBK3B_EXPORT PTT
        {
        public:
            PTT() {}

            unsigned int pttNumber() const { return m_pttNum; }

            const Time& playbackTime() const { return m_playbackTime; }

            unsigned int firstSector() const { return m_firstSector; }
            unsigned int lastSector() const { return m_lastSector; }

        private:
            unsigned int m_pttNum;
            Time m_playbackTime;

            unsigned int m_firstSector;
            unsigned int m_lastSector;

            friend class VideoDVD;
        };
    }
}

#endif
