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

#ifndef K3B_BLANKING_JOB_H
#define K3B_BLANKING_JOB_H

#include "k3bjob.h"
#include "k3b_export.h"

class QString;

namespace K3b {
    class AbstractWriter;

    class LIBK3B_EXPORT BlankingJob : public BurnJob
    {
        Q_OBJECT

    public:
        explicit BlankingJob( JobHandler*, QObject* parent = 0 );
        ~BlankingJob() override;

        QString jobDescription() const override;
        QString jobDetails() const override;

        Device::Device* writer() const override;

        bool hasBeenCanceled() const override { return m_canceled; }

    public Q_SLOTS:
        void start() override;
        void cancel() override;
        void setForce( bool f ) { m_force = f; }
        void setDevice( K3b::Device::Device* d );
        void setSpeed( int s ) { m_speed = s; }
        void setFormattingMode( FormattingMode mode ) { m_mode = mode; }
        void setWritingApp( WritingApp app ) { m_writingApp = app; }

        /**
         * If set true the job ignores the global K3b setting
         * and does not eject the CD-RW after finishing
         */
        void setForceNoEject( bool b ) { m_forceNoEject = b; }

    private Q_SLOTS:
        void slotFinished(bool);
        void slotStartErasing();

    private:
        AbstractWriter* m_writerJob;
        bool m_force;
        Device::Device* m_device;
        int m_speed;
        FormattingMode m_mode;
        WritingApp m_writingApp;
        bool m_canceled;
        bool m_forceNoEject;
    };
}

#endif
