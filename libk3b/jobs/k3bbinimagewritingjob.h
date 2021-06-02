/*
    SPDX-FileCopyrightText: 2003 Klaus-Dieter Krannich <kd@k3b.org>
    SPDX-FileCopyrightText: 2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef K3BBINIMAGEWRITINGJOB_H
#define K3BBINIMAGEWRITINGJOB_H

#include "k3bjob.h"
#include "k3b_export.h"

namespace K3b {

    class AbstractWriter;

    namespace Device {
        class Device;
    }

    /**
     * @author Klaus-Dieter Krannich
     */
    class LIBK3B_EXPORT BinImageWritingJob : public BurnJob
    {
        Q_OBJECT

    public:
        explicit BinImageWritingJob( JobHandler*, QObject* parent = 0 );
        ~BinImageWritingJob() override;

        Device::Device* writer() const override { return m_device; };

        QString jobDescription() const override;
        QString jobDetails() const override;
        QString jobSource() const override;
        QString jobTarget() const override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        void setWriter( K3b::Device::Device* dev ) { m_device = dev; }
        void setSimulate( bool b ) { m_simulate = b; }
        void setMulti( bool b ) { m_noFix = b; }
        void setTocFile( const QString& s);
        void setCopies(int c) { m_copies = c; }
        void setSpeed( int s ) { m_speed = s; }

    private Q_SLOTS:
        void writerFinished(bool);
        void copyPercent(int p);
        void copySubPercent(int p);
        void slotNextTrack( int, int );

    private:
        void writerStart();
        bool prepareWriter();

        Device::Device* m_device;
        bool m_simulate;
        bool m_force;
        bool m_noFix;
        QString m_tocFile;
        int m_speed;
        int m_copies;
        int m_finishedCopies;

        bool m_canceled;

        AbstractWriter* m_writer;
    };
}

#endif
