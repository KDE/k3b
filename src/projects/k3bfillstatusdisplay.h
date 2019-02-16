
/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BFILLSTATUSDISPLAY_H
#define K3BFILLSTATUSDISPLAY_H

#include <QMouseEvent>
#include <QPaintEvent>
#include <QFrame>

class QPaintEvent;
class QMouseEvent;
namespace K3b {
    class Doc;
    class Msf;

    namespace Device {
        class Device;
    }

    class FillStatusDisplayWidget : public QWidget
    {
        Q_OBJECT

    public:
        FillStatusDisplayWidget( Doc* doc, QWidget* parent );
        ~FillStatusDisplayWidget() override;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        const K3b::Msf& cdSize() const;

    public Q_SLOTS:
        void setShowTime( bool b );
        void setCdSize( const K3b::Msf& size );

    Q_SIGNALS:
        void contextMenu( const QPoint& );

    protected:
        void mousePressEvent( QMouseEvent* ) override;
        void paintEvent(QPaintEvent*) override;

    private:
        class Private;
        Private* d;
    };


    class FillStatusDisplay : public QFrame
    {
        Q_OBJECT

    public:
        explicit FillStatusDisplay(Doc* doc, QWidget *parent=0);
        ~FillStatusDisplay() override;

    public Q_SLOTS:
        void showSize();
        void showTime();

    protected:
        void setupPopupMenu();

    private Q_SLOTS:
        void slotAutoSize();
        void slot74Minutes();
        void slot80Minutes();
        void slot100Minutes();
        void slotDvd4_7GB();
        void slotDvdDoubleLayer();
        void slotWhy44();
        void slotBD25();
        void slotBD50();
        void slotCustomSize();
        void slotMenuButtonClicked();
        void slotPopupMenu(const QPoint&);
        void slotDetermineSize();
        void slotDocChanged();
        void slotMediumChanged( K3b::Device::Device* dev );
        void slotUpdateDisplay();

        void slotLoadUserDefaults();
        void slotSaveUserDefaults();

    protected:
        bool event( QEvent* ) override;

    private:
        class Private;
        Private* d;
    };
}

#endif
