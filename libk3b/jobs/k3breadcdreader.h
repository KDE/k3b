/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_READCD_READER_H_
#define _K3B_READCD_READER_H_

#include "k3bjob.h"

#include <QProcess>

namespace K3b {

    namespace Device {
        class Device;
    }

    class Msf;

    class ReadcdReader : public Job
    {
        Q_OBJECT

    public:
        explicit ReadcdReader( JobHandler*, QObject* parent = nullptr );
        ~ReadcdReader() override;

        bool active() const override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        void setReadDevice( K3b::Device::Device* dev ) { m_readDevice = dev; }

        /** 0 means MAX */
        void setReadSpeed( int s ) { m_speed = s; }
        void setDisableCorrection( bool b ) { m_noCorr = b; }

        /** default: true */
        void setAbortOnError( bool b ) { m_noError = !b; }
        void setC2Scan( bool b ) { m_c2Scan = b; }
        void setClone( bool b ) { m_clone = b; }
        void setRetries( int i ) { m_retries = i; }

        void setSectorRange( const Msf&, const Msf& );

        void setImagePath( const QString& p ) { m_imagePath = p; }

        /**
         * the data gets written directly into device instead of the imagefile.
         * Be aware that this only makes sense before starting the job.
         * To disable just set ioDev to 0
         */
        void writeTo( QIODevice* ioDev );

    private Q_SLOTS:
        void slotStderrLine( const QString& line );
        void slotProcessExited( int exitCode, QProcess::ExitStatus exitStatus );
        void slotReadyRead();

    private:
        bool m_noCorr;
        bool m_clone;
        bool m_noError;
        bool m_c2Scan;
        int m_speed;
        int m_retries;

        Device::Device* m_readDevice;

        QString m_imagePath;

        class Private;
        Private* d;
    };
}

#endif
