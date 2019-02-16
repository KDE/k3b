/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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


#ifndef _K3BCDCOPYJOB_H_
#define _K3BCDCOPYJOB_H_

#include "k3bjob.h"
#include "k3b_export.h"
#include "k3bglobals.h"

#include <KCddb/Kcddb>

namespace K3b {
    namespace Device {
        class Device;
        class DeviceHandler;
    }


/**
 *@author Sebastian Trueg
 */
    class LIBK3B_EXPORT CdCopyJob : public BurnJob
    {
        Q_OBJECT

    public:
        explicit CdCopyJob( JobHandler* hdl, QObject* parent = 0 );
        ~CdCopyJob() override;

        Device::Device* writer() const override { return m_onlyCreateImages ? 0 : m_writerDevice; }
        Device::Device* reader() const { return m_readerDevice; }

        QString jobDescription() const override;
        QString jobDetails() const override;
        
        QString jobSource() const override;
        QString jobTarget() const override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

    public:
        void setWriterDevice( K3b::Device::Device* dev ) { m_writerDevice = dev; }
        void setReaderDevice( K3b::Device::Device* dev ) { m_readerDevice = dev; }
        void setWritingMode( K3b::WritingMode m ) { m_writingMode = m; }
        void setSpeed( int s ) { m_speed = s; }
        void setOnTheFly( bool b ) { m_onTheFly = b; }
        void setKeepImage( bool b ) { m_keepImage = b; }
        void setOnlyCreateImage( bool b ) { m_onlyCreateImages = b; }
        void setSimulate( bool b ) { m_simulate = b; }
        void setTempPath( const QString& path ) { m_tempPath= path; }
        void setCopies( unsigned int c ) { m_copies = c; }
        void setParanoiaMode( int i ) { m_paranoiaMode = i; }
        void setIgnoreDataReadErrors( bool b ) { m_ignoreDataReadErrors = b; }
        void setDataReadRetries( int i ) { m_dataReadRetries = i; }
        void setIgnoreAudioReadErrors( bool b ) { m_ignoreAudioReadErrors = b; }
        void setAudioReadRetries( int i ) { m_audioReadRetries = i; }
        void setCopyCdText( bool b ) { m_copyCdText = b; }
        void setNoCorrection( bool b ) { m_noCorrection = b; }

    private Q_SLOTS:
        void slotDiskInfoReady( K3b::Device::DeviceHandler* );
        void slotCdTextReady( K3b::Device::DeviceHandler* );
        void slotMediaReloadedForNextSession( K3b::Device::DeviceHandler* dh );
        void slotCddbQueryFinished( KCDDB::Result );
        void slotWritingNextTrack( int t, int tt );
        void slotReadingNextTrack( int t, int tt );
        void slotSessionReaderFinished( bool success );
        void slotWriterFinished( bool success );
        void slotReaderProgress( int p );
        void slotReaderSubProgress( int p );
        void slotWriterProgress( int p );
        void slotReaderProcessedSize( int p, int pp );

    private:
        void startCopy();
        void searchCdText();
        void queryCddb();
        bool writeNextSession();
        void readNextSession();
        bool prepareImageFiles();
        void cleanup();
        void finishJob( bool canceled, bool error );

        Device::Device* m_writerDevice;
        Device::Device* m_readerDevice;
        bool m_simulate;
        int m_speed;
        int m_paranoiaMode;
        int m_copies;
        bool m_keepImage;
        bool m_onlyCreateImages;
        bool m_onTheFly;
        bool m_ignoreDataReadErrors;
        bool m_ignoreAudioReadErrors;
        bool m_noCorrection;
        int m_dataReadRetries;
        int m_audioReadRetries;
        bool m_copyCdText;
        QString m_tempPath;
        WritingMode m_writingMode;

        class Private;
        Private* d;
    };
}

#endif
