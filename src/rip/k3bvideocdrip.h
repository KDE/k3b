/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

#ifndef K3BVIDEOCDRIP_H
#define K3BVIDEOCDRIP_H

#include "k3bjob.h"
#include "k3bdiskinfo.h"
#include "k3bvideocdrippingoptions.h"

#include <QProcess>

class QString;
class KProcess;

namespace K3b {
class VideoCdRip : public Job
{
        Q_OBJECT

    public:
        VideoCdRip( JobHandler*, VideoCdRippingOptions* options, QObject* parent = 0 );
        ~VideoCdRip() override;

        enum { CDROM, BIN_IMAGE, NRG_IMAGE };

        QString jobDescription() const override;
        QString jobDetails() const override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

    private Q_SLOTS:
        void cancelAll();

    protected Q_SLOTS:
        void slotVcdXRipFinished( int, QProcess::ExitStatus );
        void slotParseVcdXRipOutput();

    private:
        void vcdxRip();
        void parseInformation( QString );

        enum { stageUnknown, stageScan, stageFinished, _stage_max };

        int m_stage;
        int m_bytesFinished;
        int m_ripsourceType;
        int m_oldpercent;

        long m_subPosition;

        QString m_collectedOutput;

        VideoCdRippingOptions * m_videooptions;

        bool m_canceled;

        KProcess* m_process;

};
}

#endif
