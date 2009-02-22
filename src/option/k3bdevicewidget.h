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


#ifndef K3BDEVICEWIDGET_H
#define K3BDEVICEWIDGET_H

#include <qwidget.h>
#include <QFrame>
#include <QLabel>
#include "k3bdevice.h"
#include "k3bdevicemanager.h"

class QLabel;
class QPushButton;
namespace K3b {
    class ListView;
}
class QString;
class QFrame;
class Q3ListViewItem;
class QString;


/**
 *@author Sebastian Trueg
 */
namespace K3b {
class DeviceWidget : public QWidget
{
    Q_OBJECT

public:
    DeviceWidget( Device::DeviceManager*, QWidget *parent = 0 );
    ~DeviceWidget();

public Q_SLOTS:
    void init();

Q_SIGNALS:
    void refreshButtonClicked();

private Q_SLOTS:
    void updateDeviceListViews();

private:
    class PrivateTempDevice;
    /** list to save changes to the devices before applying */
    QList<PrivateTempDevice*> m_tempDevices;

    Q3ListViewItem* m_writerParentViewItem;
    Q3ListViewItem* m_readerParentViewItem;

    Device::DeviceManager* m_deviceManager;

    ListView*    m_viewDevices;
    QPushButton* m_buttonRefreshDevices;
};
}

#endif
