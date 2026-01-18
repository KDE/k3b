/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3BBURNPROGRESSDIALOG_H
#define K3BBURNPROGRESSDIALOG_H

#include "k3bjobprogressdialog.h"
#include "k3bdevicetypes.h"

class QProgressBar;
class QLabel;

namespace K3b {
    class BurnJob;
    class ThemedLabel;

    /**
     *@author Sebastian Trueg
     */
    class BurnProgressDialog : public JobProgressDialog  {

        Q_OBJECT

    public:
        explicit BurnProgressDialog( QWidget* parent = nullptr, bool showSubProgress = true );
        ~BurnProgressDialog() override;

        void setJob( Job* ) override;
        void setBurnJob( BurnJob* );

    protected Q_SLOTS:
        void slotWriteSpeed( int, K3b::Device::SpeedMultiplicator );
        void slotBufferStatus( int );
        void slotDeviceBuffer( int );
        void slotFinished(bool) override;

    protected:
        ThemedLabel* m_labelWriter;
        QProgressBar* m_progressWritingBuffer;
        QProgressBar* m_progressDeviceBuffer;
        QLabel* m_labelWritingSpeed;
    };
}

#endif
