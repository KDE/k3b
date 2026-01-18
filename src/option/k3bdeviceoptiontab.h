/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef K3B_DEVICE_OPTIONTAB_H
#define K3B_DEVICE_OPTIONTAB_H

#include <QWidget>

namespace K3b {
    class DeviceWidget;

    class DeviceOptionTab : public QWidget
    {
        Q_OBJECT

    public:
        explicit DeviceOptionTab( QWidget* = nullptr );
        ~DeviceOptionTab() override;

        void readDevices();
        void saveDevices();

    private Q_SLOTS:
        void slotRefreshButtonClicked();

    private:
        DeviceWidget* m_deviceWidget;
    };
}



#endif
