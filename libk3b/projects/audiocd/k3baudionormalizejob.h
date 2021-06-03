/*
    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_AUDIO_NORMALIZE_JOB_H_
#define _K3B_AUDIO_NORMALIZE_JOB_H_


#include "k3bjob.h"

#include <QList>
#include <QProcess>

namespace K3b {
    class Process;

    class AudioNormalizeJob : public Job
    {
        Q_OBJECT

    public:
        explicit AudioNormalizeJob( JobHandler*, QObject* parent = 0 );
        ~AudioNormalizeJob() override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

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
