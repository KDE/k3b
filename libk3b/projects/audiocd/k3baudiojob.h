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


#ifndef K3BAUDIOJOB_H
#define K3BAUDIOJOB_H

#include "k3bjob.h"

class QTemporaryFile;

namespace K3b {
    class AudioDoc;
    class AudioImager;
    class AbstractWriter;
    class AudioNormalizeJob;
    class AudioJobTempData;
    class AudioMaxSpeedJob;
    class Doc;

    /**
     *@author Sebastian Trueg
     */
    class AudioJob : public BurnJob
    {
        Q_OBJECT

    public:
        AudioJob( AudioDoc*, JobHandler*, QObject* parent = 0 );
        ~AudioJob() override;

        Doc* doc() const;
        Device::Device* writer() const override;

        QString jobDescription() const override;
        QString jobDetails() const override;

    public Q_SLOTS:
        void cancel() override;
        void start() override;

    protected Q_SLOTS:
        // writer slots
        void slotWriterFinished( bool success );
        void slotWriterNextTrack(int, int);
        void slotWriterJobPercent(int);

        // audiodecoder slots
        void slotAudioDecoderFinished( bool );
        void slotAudioDecoderNextTrack( int, int );
        void slotAudioDecoderPercent(int);
        void slotAudioDecoderSubPercent( int );

        // normalizing slots
        void slotNormalizeJobFinished( bool );
        void slotNormalizeProgress( int );
        void slotNormalizeSubProgress( int );

        // max speed
        void slotMaxSpeedJobFinished( bool );

    private:
        bool prepareWriter();
        bool startWriting();
        void cleanupAfterError();
        void removeBufferFiles();
        void normalizeFiles();
        bool writeTocFile();
        bool writeInfFiles();
        bool checkAudioSources();

        AudioDoc* m_doc;
        AudioImager* m_audioImager;
        AbstractWriter* m_writer;
        AudioNormalizeJob* m_normalizeJob;
        AudioJobTempData* m_tempData;
        AudioMaxSpeedJob* m_maxSpeedJob;

        QTemporaryFile* m_tocFile;

        bool m_canceled;
        bool m_errorOccuredAndAlreadyReported;

        bool m_written;

        WritingApp m_usedWritingApp;
        WritingMode m_usedWritingMode;

        class Private;
        Private* d;
    };
}

#endif
