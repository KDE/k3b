/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_CUE_FILEWRITING_JOB_H_
#define _K3B_AUDIO_CUE_FILEWRITING_JOB_H_

#include "k3bjob.h"
#include "k3b_export.h"
#include "k3bglobals.h"

namespace K3b {

    namespace Device {
        class Device;
    }


    class LIBK3B_EXPORT AudioCueFileWritingJob : public BurnJob
    {
        Q_OBJECT

    public:
        explicit AudioCueFileWritingJob( JobHandler*, QObject* parent = 0 );
        ~AudioCueFileWritingJob() override;

        Device::Device* writer() const override;

        QString jobDescription() const override;
        QString jobDetails() const override;
        QString jobSource() const override;
        QString jobTarget() const override;

        QString cueFile() const;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        void setCueFile( const QString& );
        void setSpeed( int s );
        void setBurnDevice( K3b::Device::Device* dev );
        void setWritingMode( K3b::WritingMode mode );
        void setSimulate( bool b );
        void setCopies( int c );
        void setOnTheFly( bool b );
        void setTempDir( const QString& );

    private Q_SLOTS:
        void slotAnalyserJobFinished(bool);

    private:
        void importCueInProject();

        class Private;
        Private* const d;
    };
}

#endif
