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


#ifndef K3BMIXEDJOB_H
#define K3BMIXEDJOB_H

#include "k3bjob.h"

class QTemporaryFile;

namespace K3b {
    
    namespace Device {
        class DeviceHandler;
    }
    class MixedDoc;
    class IsoImager;
    class AudioImager;
    class AbstractWriter;
    class WaveFileWriter;
    class CdrecordWriter;
    class MsInfoFetcher;
    class AudioNormalizeJob;
    class AudioJobTempData;
    class Doc;

    class MixedJob : public BurnJob
    {
        Q_OBJECT

    public:
        MixedJob( MixedDoc*, JobHandler*, QObject* parent = 0 );
        ~MixedJob() override;

        Doc* doc() const;
        Device::Device* writer() const override;

        QString jobDescription() const override;
        QString jobDetails() const override;

    public Q_SLOTS:
        void cancel() override;
        void start() override;

    protected Q_SLOTS:
        // iso imager slots
        void slotIsoImagerFinished( bool success );
        void slotIsoImagerPercent(int);

        // ms info fetcher slots
        void slotMsInfoFetched(bool);

        // audio decoder slots
        void slotAudioDecoderFinished( bool );
        void slotAudioDecoderNextTrack( int, int );
        void slotAudioDecoderPercent(int);
        void slotAudioDecoderSubPercent( int );

        // writer slots
        void slotWriterFinished( bool success );
        void slotWriterNextTrack(int, int);
        void slotWriterJobPercent(int);

        // normalizing slots
        void slotNormalizeJobFinished( bool );
        void slotNormalizeProgress( int );
        void slotNormalizeSubProgress( int );

        // misc slots
        void slotMediaReloadedForSecondSession( K3b::Device::DeviceHandler* dh );
        void slotMaxSpeedJobFinished( bool );

    private:
        bool prepareWriter();
        bool writeTocFile();
        bool writeInfFiles();
        bool startWriting();
        void startFirstCopy();
        void startSecondSession();
        void addAudioTracks( CdrecordWriter* writer );
        void addDataTrack( CdrecordWriter* writer );
        void cleanupAfterError();
        void removeBufferFiles();
        void createIsoImage();
        void determineWritingMode();
        void normalizeFiles();
        void prepareProgressInformation();
        void writeNextCopy();
        void determinePreliminaryDataImageSize();

        MixedDoc* m_doc;
        IsoImager* m_isoImager;
        AudioImager* m_audioImager;
        AudioJobTempData* m_tempData;
        WaveFileWriter* m_waveFileWriter;
        AbstractWriter* m_writer;
        MsInfoFetcher* m_msInfoFetcher;
        AudioNormalizeJob* m_normalizeJob;

        QString m_isoImageFilePath;

        QTemporaryFile* m_tocFile;

        enum Action { INITIALIZING_IMAGER,
                      PREPARING_DATA,
                      CREATING_ISO_IMAGE,
                      CREATING_AUDIO_IMAGE,
                      WRITING_ISO_IMAGE,
                      WRITING_AUDIO_IMAGE,
                      FETCHING_MSMessageInfo };

        int m_currentAction;
        double m_audioDocPartOfProcess;

        bool m_canceled;
        bool m_errorOccuredAndAlreadyReported;

        int m_usedDataMode;
        WritingApp m_usedDataWritingApp;
        WritingApp m_usedAudioWritingApp;
        WritingMode m_usedDataWritingMode;
        WritingMode m_usedAudioWritingMode;

        QString m_tempFilePrefix;

        Msf m_projectSize;

        class Private;
        Private* d;
    };
}

#endif
