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

#ifndef _K3B_AUDIO_CDTRACK_DRAG_H_
#define _K3B_AUDIO_CDTRACK_DRAG_H_

#include <q3dragobject.h>
#include <q3cstring.h>
#include <q3valuelist.h>

#include <k3btoc.h>
#include <k3bdevice.h>
#include "k3b_export.h"

#include <libkcddb/cdinfo.h>


class LIBK3B_EXPORT K3bAudioCdTrackDrag : public Q3StoredDrag
{
public:
    K3bAudioCdTrackDrag( const K3bDevice::Toc& toc, const QList<int>& cdTrackNumbers, const KCDDB::CDInfo& cddb,
                         K3bDevice::Device* lastDev = 0, QWidget* dragSource = 0 );

    K3bDevice::Toc toc() const { return m_toc; }
    QList<int> cdTrackNumbers() const { return m_cdTrackNumbers; }
    KCDDB::CDInfo cddbEntry() const { return m_cddb; }

    bool provides( const char* mimetype ) const { return !qstrcmp( mimetype, "k3b/audio_track_drag" ); }

    static bool canDecode( const QMimeSource* s ) { return s->provides( "k3b/audio_track_drag" ); }
    static bool decode( const QMimeSource* s, K3bDevice::Toc&, QList<int>& trackNumbers, KCDDB::CDInfo&, K3bDevice::Device** dev = 0 );

private:
    K3bDevice::Toc m_toc;
    QList<int> m_cdTrackNumbers;
    KCDDB::CDInfo m_cddb;
    K3bDevice::Device* m_device;
};

#endif
