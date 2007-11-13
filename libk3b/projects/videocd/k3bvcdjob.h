/*
*
* $Id$
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3BVCDJOB_H
#define K3BVCDJOB_H

#include <k3bjob.h>

class K3bVcdDoc;
class K3bVcdTrack;
class QString;
class K3bProcess;
class K3Process;
class QDataStream;
class K3bAbstractWriter;
class K3bDevice::Device;


class K3bVcdJob : public K3bBurnJob
{
        Q_OBJECT

    public:
        K3bVcdJob( K3bVcdDoc*, K3bJobHandler*, QObject* parent = 0 );
        ~K3bVcdJob();

        K3bDoc* doc() const;
        K3bVcdDoc* vcdDoc() const
        {
            return m_doc;
        }
        K3bDevice::Device* writer() const;

        QString jobDescription() const;
        QString jobDetails() const;

    public slots:
        void start();
        void cancel();

    private slots:
        void cancelAll();

    protected slots:
        void slotVcdxBuildFinished();
        void slotParseVcdxBuildOutput( K3Process*, char* output, int len );

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

        K3bVcdDoc* m_doc;
        K3bDevice::Device* m_writer;
        K3bDevice::Device* m_reader;
        K3bVcdTrack* m_currentWrittenTrack;

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
        QString m_xmlFile;
        QString m_collectedOutput;

        K3bAbstractWriter* m_writerJob;
        K3bProcess* m_process;
};

#endif
