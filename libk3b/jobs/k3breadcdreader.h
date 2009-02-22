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


#ifndef _K3B_READCD_READER_H_
#define _K3B_READCD_READER_H_

#include <k3bjob.h>

#include <qprocess.h>

namespace K3b {

    namespace Device {
        class Device;
    }

    class Msf;

    class ReadcdReader : public Job
    {
        Q_OBJECT

    public:
        ReadcdReader( JobHandler*, QObject* parent = 0 );
        ~ReadcdReader();

        bool active() const;

    public Q_SLOTS:
        void start();
        void cancel();

        void setReadDevice( Device::Device* dev ) { m_readDevice = dev; }

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
         * the data gets written directly into fd instead of the imagefile.
         * Be aware that this only makes sense before starting the job.
         * To disable just set fd to -1
         */
        void writeToFd( int fd );

    private Q_SLOTS:
        void slotStdLine( const QString& line );
        void slotProcessExited( int exitCode, QProcess::ExitStatus exitStatus );

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
