/*
 *
 * Copyright (C) 2008-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DISKINFO_P_H_
#define _K3B_DISKINFO_P_H_

#include <QtCore/QByteArray>
#include <QtCore/QSharedData>

#include "k3bmsf.h"
#include "k3bdeviceglobals.h"

namespace K3b {
    namespace Device {
        class DiskInfoPrivate : public QSharedData
        {
        public:
            DiskInfoPrivate()
                : mediaType(MEDIA_UNKNOWN),
                  currentProfile(MEDIA_UNKNOWN),
                  diskState(STATE_UNKNOWN),
                  lastSessionState(STATE_UNKNOWN),
                  bgFormatState(BG_FORMAT_INVALID),
                  numSessions(0),
                  numTracks(0),
                  rewritable(false) {
            }

            MediaType mediaType;
            int currentProfile;

            MediaState diskState;
            MediaState lastSessionState;
            BackGroundFormattingState bgFormatState;
            int numSessions;
            int numTracks;
            int numLayers;  // only for DVD media
            int rewritable;

            K3b::Msf capacity;
            K3b::Msf usedCapacity;
            K3b::Msf firstLayerSize;

            QByteArray mediaId;
        };
    }
}

#endif
