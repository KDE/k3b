/*
    SPDX-FileCopyrightText: 2003 Christian Kvasny <chris@k3b.org>
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
        VideoCdRip( JobHandler*, VideoCdRippingOptions* options, QObject* parent = nullptr );
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
