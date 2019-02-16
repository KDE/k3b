/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_DVD_FORMATTING_JOB_H_
#define _K3B_DVD_FORMATTING_JOB_H_

#include "k3bjob.h"
#include "k3b_export.h"
#include <QProcess>

namespace K3b {
    namespace Device {
        class Device;
        class DeviceHandler;
    }

    /**
     * Formats and blanks DVD-RW, DVD+RW and BD-RE discs
     */
    class LIBK3B_EXPORT DvdFormattingJob : public BurnJob
    {
        Q_OBJECT

    public:
        explicit DvdFormattingJob( JobHandler*, QObject* parent = 0 );
        ~DvdFormattingJob() override;

        QString jobDescription() const override;
        QString jobDetails() const override;

        Device::Device* writer() const override;

    public Q_SLOTS:
        void start() override;

        /**
         * Use this to force the start of the formatting without checking for a usable medium.
         */
        void start( const K3b::Device::DiskInfo& );

        void cancel() override;

        void setDevice( K3b::Device::Device* );

        /**
         * One of: WritingModeIncrementalSequential, WritingModeRestrictedOverwrite
         * Ignored for DVD+RW and BD-RE
         */
        void setMode( int );

        /**
         * Not all writers supports quick mode
         */
        void setFormattingMode( FormattingMode mode );

        /**
         * @param b If true empty discs will also be formatted
         */
        void setForce( bool b );

        /**
         * If set true the job ignores the global K3b setting
         * and does not eject the CD-RW after finishing
         */
        void setForceNoEject( bool );

    private Q_SLOTS:
        void slotStderrLine( const QString& );
        void slotProcessFinished( int, QProcess::ExitStatus );
        void slotDeviceHandlerFinished( K3b::Device::DeviceHandler* );
        void slotEjectingFinished( K3b::Device::DeviceHandler* );

    private:
        void startFormatting( const Device::DiskInfo& );

        class Private;
        Private* d;
    };
}


#endif
