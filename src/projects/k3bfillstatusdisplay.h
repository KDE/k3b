
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
class K3bDoc;

namespace K3bDevice {
    class Device;
}
namespace K3b {
    class Msf;
}


/**
 *@author Sebastian Trueg
 */
class K3bFillStatusDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    K3bFillStatusDisplayWidget( K3bDoc* doc, QWidget* parent );
    ~K3bFillStatusDisplayWidget();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    const K3b::Msf& cdSize() const;

public Q_SLOTS:
    void setShowTime( bool b );
    void setCdSize( const K3b::Msf& );

Q_SIGNALS:
    void contextMenu( const QPoint& );

protected:
    void mousePressEvent( QMouseEvent* );
    void paintEvent(QPaintEvent*);

private:
    class Private;
    Private* d;
};


class K3bFillStatusDisplay : public QFrame
{
    Q_OBJECT

public:
    K3bFillStatusDisplay(K3bDoc* doc, QWidget *parent=0);
    ~K3bFillStatusDisplay();

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
    void slotMediumChanged( K3bDevice::Device* dev );
    void slotUpdateDisplay();

    void slotLoadUserDefaults();
    void slotSaveUserDefaults();

protected:
    bool event( QEvent* );

private:
    class Private;
    Private* d;
};

#endif
