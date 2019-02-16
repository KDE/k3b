/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
* Copyright (C) 2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef K3BVCDJOB_H
#define K3BVCDJOB_H

#include "k3bjob.h"
#include <QProcess>

class K3Process;

namespace K3b {
    class VcdDoc;
    class VcdTrack;
    class Process;
    class AbstractWriter;
    class Doc;

    class VcdJob : public BurnJob
    {
        Q_OBJECT

    public:
        VcdJob( VcdDoc*, JobHandler*, QObject* parent = 0 );
        ~VcdJob() override;

        Doc* doc() const;
        VcdDoc* vcdDoc() const
        {
            return m_doc;
        }
        Device::Device* writer() const override;

        QString jobDescription() const override;
        QString jobDetails() const override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

    private Q_SLOTS:
        void cancelAll();

    protected Q_SLOTS:
        void slotVcdxBuildFinished( int, QProcess::ExitStatus );
        void slotParseVcdxBuildOutput( const QString& );

        void slotWriterJobPercent( int p );
        void slotProcessedSize( int cs, int ts );
        void slotWriterNextTrack( int t, int tt );
        void slotWriterJobFinished( bool success );


    private:
        bool prepareWriterJob();

        void xmlGen();
        void vcdxBuild();
        void parseInformation( const QString& );
        void startWriterjob();

        int m_copies;
        int m_finishedCopies;

        unsigned long m_blocksToCopy;
        unsigned long m_bytesFinishedTracks;
        unsigned long m_bytesFinished;

        enum { stageUnknown, stageScan, stageWrite, _stage_max };

        VcdDoc* m_doc;
        Device::Device* m_writer;
        Device::Device* m_reader;
        VcdTrack* m_currentWrittenTrack;

        int m_speed;
        int m_stage;
        int m_currentcopy;
        int m_currentWrittenTrackNumber;

        double m_createimageonlypercent;

        bool firstTrack;
        bool m_burnProof;
        bool m_keepImage;
        bool m_onlyCreateImage;
        bool m_onTheFly;
        bool m_dummy;
        bool m_fastToc;
        bool m_readRaw;
        bool m_imageFinished;
        bool m_canceled;

        QString m_tempPath;
        QString m_cueFile;
        QString m_collectedOutput;

        AbstractWriter* m_writerJob;
        Process* m_process;
        
        class Private;
        Private* d;
    };
}

#endif
