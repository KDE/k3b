/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010-2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef K3B_AUDIO_PROJECT_CONVERTING_JOB_H
#define K3B_AUDIO_PROJECT_CONVERTING_JOB_H

#include "k3bmassaudioencodingjob.h"
#include <QScopedPointer>

namespace K3b {

class AudioDoc;
    
class AudioProjectConvertingJob : public MassAudioEncodingJob
{
    Q_OBJECT

public:
    AudioProjectConvertingJob( AudioDoc* doc, JobHandler* hdl, QObject* parent );
    ~AudioProjectConvertingJob() override;

    QString jobDescription() const override;

private:
    bool init() override;

    Msf trackLength( int trackIndex ) const override;

    QIODevice* createReader( int trackIndex ) const override;

    void trackStarted( int trackIndex ) override;
    
    void trackFinished( int trackIndex, const QString& filename ) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace K3b

#endif
