/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_CLONE_JOB_H_
#define _K3B_CLONE_JOB_H_

#include "k3bjob.h"
#include "k3b_export.h"
#include <QString>


namespace K3b {
    namespace Device {
        class Device;
    }
    class CdrecordWriter;
    class ReadcdReader;

    class LIBK3B_EXPORT CloneJob : public BurnJob
    {
        Q_OBJECT

    public:
        explicit CloneJob( JobHandler*, QObject* parent = 0 );
        ~CloneJob() override;

        Device::Device* writer() const override { return m_writerDevice; }
        Device::Device* readingDevice() const { return m_readerDevice; }

        QString jobDescription() const override;
        QString jobDetails() const override;
        QString jobSource() const override;
        QString jobTarget() const override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        void setWriterDevice( K3b::Device::Device* w ) { m_writerDevice = w; }
        void setReaderDevice( K3b::Device::Device* w ) { m_readerDevice = w; }
        void setImagePath( const QString& p ) { m_imagePath = p; }
        void setNoCorrection( bool b ) { m_noCorrection = b; }
        void setRemoveImageFiles( bool b ) { m_removeImageFiles = b; }
        void setOnlyCreateImage( bool b ) { m_onlyCreateImage = b; }
        void setOnlyBurnExistingImage( bool b ) { m_onlyBurnExistingImage = b; }
        void setSimulate( bool b ) { m_simulate = b; }
        void setWriteSpeed( int s ) { m_speed = s; }
        void setCopies( int c ) { m_copies = c; }
        void setReadRetries( int i ) { m_readRetries = i; }

    private Q_SLOTS:
        void slotWriterPercent( int );
        void slotWriterFinished( bool );
        void slotWriterNextTrack( int, int );
        void slotReadingPercent( int );
        void slotReadingFinished( bool );

    private:
        void removeImageFiles();
        void prepareReader();
        void prepareWriter();
        void startWriting();

        Device::Device* m_writerDevice;
        Device::Device* m_readerDevice;
        QString m_imagePath;

        CdrecordWriter* m_writerJob;
        ReadcdReader* m_readcdReader;

        bool m_noCorrection;
        bool m_removeImageFiles;

        bool m_canceled;
        bool m_running;

        bool m_simulate;
        int m_speed;
        int m_copies;
        bool m_onlyCreateImage;
        bool m_onlyBurnExistingImage;
        int m_readRetries;

        class Private;
        Private* d;
    };
}

#endif
