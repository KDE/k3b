/*
    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_BUSY_WIDGET_H
#define K3B_BUSY_WIDGET_H


#include "k3b_export.h"
#include <QFrame>

class QTimer;


namespace K3b {
    class LIBK3B_EXPORT BusyWidget : public QFrame
    {
        Q_OBJECT

    public:
        explicit BusyWidget( QWidget* parent = 0 );
        ~BusyWidget() override;

        void showBusy( bool b );

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void paintEvent( QPaintEvent* ) override;

    private Q_SLOTS:
        void animateBusy();

    private:
        bool m_bBusy;
        int m_iBusyPosition;

        QTimer* m_busyTimer;
    };
}


#endif
