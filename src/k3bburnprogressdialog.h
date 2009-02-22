/* 
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BBURNPROGRESSDIALOG_H
#define K3BBURNPROGRESSDIALOG_H

#include "k3bjobprogressdialog.h"

namespace K3b {
    class BurnJob;
}
namespace K3b {
    class ThemedLabel;
}
class QProgressBar;
class QLabel;


/**
 *@author Sebastian Trueg
 */
namespace K3b {
class BurnProgressDialog : public JobProgressDialog  {

    Q_OBJECT

public:
    BurnProgressDialog( QWidget* parent = 0, bool showSubProgress = true );
    ~BurnProgressDialog();

    void setJob( Job* );
    void setBurnJob( BurnJob* );

protected Q_SLOTS:
    void slotWriteSpeed( int, int );
    void slotBufferStatus( int );
    void slotDeviceBuffer( int );
    void slotFinished(bool);

protected:
    ThemedLabel* m_labelWriter;
    QProgressBar* m_progressWritingBuffer;
    QProgressBar* m_progressDeviceBuffer;
    QLabel* m_labelWritingSpeed;
};
}

#endif
