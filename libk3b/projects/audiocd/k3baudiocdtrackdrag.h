/*
    SPDX-FileCopyrightText: 2005-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_CDTRACK_DRAG_H_
#define _K3B_AUDIO_CDTRACK_DRAG_H_

#include "k3btoc.h"
#include "k3bdevice.h"
#include "k3b_export.h"

#include <KCDDB/CDInfo>

class QMimeData;

namespace K3b {
    class LIBK3B_EXPORT AudioCdTrackDrag
    {
    public:
        AudioCdTrackDrag();
        AudioCdTrackDrag( const Device::Toc& toc,
                          const QList<int>& trackNumbers,
                          const KCDDB::CDInfo& cddb,
                          Device::Device* lastDev = 0 );

        Device::Toc toc() const { return m_toc; }
        QList<int> trackNumbers() const { return m_trackNumbers; }
        KCDDB::CDInfo cddbEntry() const { return m_cddb; }
        Device::Device* device() const { return m_device; }

        void populateMimeData( QMimeData* );

        static QStringList mimeDataTypes();

        static bool canDecode( const QMimeData* s );
        static AudioCdTrackDrag fromMimeData( const QMimeData* s );

    private:
        Device::Toc m_toc;
        QList<int> m_trackNumbers;
        KCDDB::CDInfo m_cddb;
        Device::Device* m_device;
    };
}

#endif
