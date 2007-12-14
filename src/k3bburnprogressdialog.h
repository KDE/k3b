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

#include <k3bjobprogressdialog.h>
//Added by qt3to4:
#include <QLabel>

class K3bBurnJob;
class QProgressBar;
class QLabel;


/**
 *@author Sebastian Trueg
 */
class K3bBurnProgressDialog : public K3bJobProgressDialog  {

    Q_OBJECT

public:
    K3bBurnProgressDialog( QWidget* parent = 0, bool showSubProgress = true );
    ~K3bBurnProgressDialog();

    void setJob( K3bJob* );
    void setBurnJob( K3bBurnJob* );

protected slots:
    void slotWriteSpeed( int, int );
    void slotBufferStatus( int );
    void slotDeviceBuffer( int );
    void slotFinished(bool);

protected:
    QLabel* m_labelWriter;
    QProgressBar* m_progressWritingBuffer;
    QProgressBar* m_progressDeviceBuffer;
    QLabel* m_labelWritingSpeed;
};

#endif
