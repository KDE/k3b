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
        explicit BurnProgressDialog( QWidget* parent = 0, bool showSubProgress = true );
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
