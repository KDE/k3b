/*
 *
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#ifndef _K3B_JOB_INTERFACE_H_
#define _K3B_JOB_INTERFACE_H_

#include <QObject>

/**
 * A D-BUS interface for K3b's currently running job.
 */
namespace K3b {
    class Job;

    class JobInterface : public QObject
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.k3b.Job" )

    public:
        explicit JobInterface( Job* job );
        ~JobInterface() override;

    public Q_SLOTS:
        bool jobRunning() const;

        QString jobDescription() const;
        QString jobDetails() const;

    Q_SIGNALS:
        void started();
        void canceled();
        void finished( bool );
        void infoMessage( const QString&, int );
        void progress( int );
        void subProgress( int );
        void newTask( const QString& );
        void newSubTask( const QString& );
        void buffer( int );
        void deviceBuffer( int );
        void nextTrack( int track, int numTracks );

    private Q_SLOTS:
        void slotProgress( int );
        void slotSubProgress( int );

    private:
        Job* m_job;

        int m_lastProgress;
        int m_lastSubProgress;
    };
}

#endif
