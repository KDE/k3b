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

#ifndef _K3B_MUSICBRAINZ_TRACK_LOOKUP_JOB_H_
#define _K3B_MUSICBRAINZ_TRACK_LOOKUP_JOB_H_

#include "k3bthreadjob.h"

class K3bAudioTrack;

class K3bMusicBrainzTrackLookupJob : public K3bThreadJob
{
    Q_OBJECT

public:
    K3bMusicBrainzTrackLookupJob( K3bJobHandler* hdl, QObject* parent );
    ~K3bMusicBrainzTrackLookupJob();

    void setAudioTrack( K3bAudioTrack* track );

    int results();
    QString title( int i ) const;
    QString artist( int i ) const;

private:
    bool run();

    class Private;
    Private* const d;
};

#endif
