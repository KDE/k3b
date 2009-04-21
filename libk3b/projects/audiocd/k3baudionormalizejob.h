/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_AUDIO_NORMALIZE_JOB_H_
#define _K3B_AUDIO_NORMALIZE_JOB_H_


#include "k3bjob.h"

#include <qprocess.h>
#include <qlist.h>

namespace K3b {
    class Process;

    class AudioNormalizeJob : public Job
    {
        Q_OBJECT

    public:
        AudioNormalizeJob( JobHandler*, QObject* parent = 0 );
        ~AudioNormalizeJob();

    public Q_SLOTS:
        void start();
        void cancel();

        void setFilesToNormalize( const QList<QString>& files ) { m_files = files; }

    private Q_SLOTS:
        void slotStdLine( const QString& line );
        void slotProcessExited( int exitCode, QProcess::ExitStatus exitStatus );

    private:
        Process* m_process;

        QList<QString> m_files;
        bool m_canceled;

        enum Action {
            COMPUTING_LEVELS,
            ADJUSTING_LEVELS
        };

        int m_currentAction;
        int m_currentTrack;
    };
}


#endif
