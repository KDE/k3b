/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3BVIDEOCDRIP_H
#define K3BVIDEOCDRIP_H

#include <k3bjob.h>
#include <device/k3bdiskinfo.h>

class QString;
class KProcess;
class QDataStream;

class K3bVideoCdRip : public K3bJob
{
    Q_OBJECT

    public:
        K3bVideoCdRip( QObject* parent = 0, const char* name = 0 );
        ~K3bVideoCdRip();

        void setDestination( QString dest) { m_destPath = dest; };
        void setVideoCdSize( long s ) {m_videocdsize = s; };

    public slots:
        void start();
        void cancel();

    private slots:
        void cancelAll();

    protected slots:
        void slotVcdXRipFinished();
        void slotParseVcdXRipOutput( KProcess*, char* output, int len );

    private:
        void vcdxRip( QString );
        void parseInformation( QString );
    
        enum { stageUnknown, stageScan, stageFinished, _stage_max };

        int m_stage;
        int m_bytesFinished;
        long m_subPosition;
        long m_videocdsize;
        
        QString m_collectedOutput;
        QString m_destPath;

        KProcess* m_process;
        bool m_canceled;
};

#endif
