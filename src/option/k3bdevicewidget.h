/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef K3BDEVICEWIDGET_H
#define K3BDEVICEWIDGET_H

#include "config-k3b.h"
#include <QWidget>

class KMessageWidget;
class QAction;
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
        explicit DeviceWidget( Device::DeviceManager*, QWidget *parent = 0 );
        ~DeviceWidget() override;

    public Q_SLOTS:
        void init();

    Q_SIGNALS:
        void refreshButtonClicked();

    private Q_SLOTS:
        void updateDeviceListViews();
        void addUserToGroup();

    private:
        Device::DeviceManager* m_deviceManager;

        QTreeWidgetItem* m_writerParentViewItem;
        QTreeWidgetItem* m_readerParentViewItem;

        QTreeWidget* m_viewDevices;
        KMessageWidget* m_messageWidget;
        QAction* m_addToGroupAction;
        QString m_deviceGroup;
    };
}

#endif
