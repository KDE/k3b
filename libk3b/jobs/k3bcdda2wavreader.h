/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef _K3B_CDDA2WAV_READER_H_
#define _K3B_CDDA2WAV_READER_H_

#include "k3bjob.h"
#include <QProcess>

namespace K3b {
    namespace Device {
        class Device;
    }


    /**
     * An Audio CD reader completely based on cdda2wav.
     * It does not use Device::Device but parses the track offsets
     * from the cdda2wav output.
     */
    class Cdda2wavReader : public Job
    {
        Q_OBJECT

    public:
        explicit Cdda2wavReader( QObject* parent = 0 );
        ~Cdda2wavReader();

        bool active() const;

    public Q_SLOTS:
        void start();
        void start( bool onlyReadInfo );
        void cancel();

        void setReadDevice( K3b::Device::Device* dev ) { m_device = dev; }
        void setImagePath( const QString& p ) { m_imagePath = p; }

        /**
         * the data gets written directly into fd instead of the imagefile.
         * Be aware that this only makes sense before starting the job.
         * To disable just set fd to -1
         */
        void writeToFd( int fd );

    private Q_SLOTS:
        void slotProcessLine( const QString& );
        void slotProcessExited( int, QProcess::ExitStatus );

    private:
        Device::Device* m_device;

        QString m_imagePath;

        class Private;
        Private* d;
    };
}

#endif
