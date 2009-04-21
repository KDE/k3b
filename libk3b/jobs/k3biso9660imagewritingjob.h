/*
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


#ifndef K3BISO9660_IMAGE_WRITING_JOB_H
#define K3BISO9660_IMAGE_WRITING_JOB_H

#include "k3bjob.h"
#include "k3b_export.h"

class QString;
class KTemporaryFile;

namespace K3b {

    class MetaWriter;
    class VerificationJob;

    namespace Device {
        class Device;
    }

    class LIBK3B_EXPORT Iso9660ImageWritingJob : public BurnJob
    {
        Q_OBJECT

    public:
        Iso9660ImageWritingJob( JobHandler* );
        ~Iso9660ImageWritingJob();

        Device::Device* writer() const { return m_device; };

        QString jobDescription() const;
        QString jobDetails() const;

    public Q_SLOTS:
        void cancel();
        void start();

        void setImagePath( const QString& path ) { m_imagePath = path; }
        void setSpeed( int s ) { m_speed = s; }
        void setBurnDevice( K3b::Device::Device* dev ) { m_device = dev; }
        void setWritingMode( K3b::WritingMode mode ) { m_writingMode = mode; }
        void setSimulate( bool b ) { m_simulate = b; }
        void setNoFix( bool b ) { m_noFix = b; }
        void setDataMode( int m ) { m_dataMode = m; }
        void setVerifyData( bool b ) { m_verifyData = b; }
        void setCopies( int c ) { m_copies = c; }

    protected Q_SLOTS:
        void slotWriterJobFinished( bool );
        void slotVerificationFinished( bool );
        void slotVerificationProgress( int );
        void slotWriterPercent( int );
        void slotNextTrack( int, int );
        void startWriting();

    private:
        bool prepareWriter( Device::MediaTypes mediaType );

        WritingMode m_writingMode;
        bool m_simulate;
        Device::Device* m_device;
        bool m_noFix;
        int m_speed;
        int m_dataMode;
        bool m_verifyData;
        bool m_dvd;

        QString m_imagePath;

        MetaWriter* m_writer;
        KTemporaryFile* m_tocFile;

        bool m_canceled;
        bool m_finished;

        int m_copies;
        int m_currentCopy;

        VerificationJob* m_verifyJob;

        class Private;
        Private* d;
    };
}

#endif
