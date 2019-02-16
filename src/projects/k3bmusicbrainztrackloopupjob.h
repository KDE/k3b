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

#ifndef _K3B_MUSICBRAINZ_TRACK_LOOPUP_JOB_H_
#define _K3B_MUSICBRAINZ_TRACK_LOOPUP_JOB_H_

#include "k3bthreadjob.h"

namespace K3b {
    class AudioTrack;
}

namespace K3b {
class MusicBrainzTrackLookupJob : public ThreadJob
{
    Q_OBJECT

public:
    MusicBrainzTrackLookupJob( JobHandler* hdl, QObject* parent );
    ~MusicBrainzTrackLookupJob() override;

    void setAudioTrack( AudioTrack* track );

    int results();
    QString title( int i ) const;
    QString artist( int i ) const;

private:
    bool run() override;

    class Private;
    Private* const d;
};
}

#endif
