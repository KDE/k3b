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


#ifndef K3BDEVICEWIDGET_H
#define K3BDEVICEWIDGET_H

#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;


namespace K3b {
    namespace Device {
        class DeviceManager;
    }

    class DeviceWidget : public QWidget
    {
        Q_OBJECT

    public:
        DeviceWidget( Device::DeviceManager*, QWidget *parent = 0 );
        ~DeviceWidget();

    public Q_SLOTS:
        void init();

    Q_SIGNALS:
        void modifyPermissionsButtonClicked();
        void refreshButtonClicked();

    private Q_SLOTS:
        void updateDeviceListViews();

    private:
        Device::DeviceManager* m_deviceManager;

        QTreeWidgetItem* m_writerParentViewItem;
        QTreeWidgetItem* m_readerParentViewItem;

        QTreeWidget* m_viewDevices;
    };
}

#endif
