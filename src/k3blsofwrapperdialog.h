/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_LSOF_WRAPPER_DIALOG_H_
#define _K3B_LSOF_WRAPPER_DIALOG_H_

#include <QDialog>

class QLabel;

namespace K3b {

    namespace Device {
        class Device;
    }

    class LsofWrapperDialog : public QDialog
    {
        Q_OBJECT

    public:
        ~LsofWrapperDialog() override;

        /**
         * Check if other applications are currently using the device and if so
         * warn the user and provide a quick solution to shut down these other
         * applications.
         *
         * If the device is not in use this method simply returns.
         */
        static void checkDevice( Device::Device* dev, QWidget* parent = 0 );

    private Q_SLOTS:
        bool slotCheckDevice();
        void slotQuitOtherApps();

    private:
        explicit LsofWrapperDialog( QWidget* parent );

        Device::Device* m_device;
        QLabel* m_label;
    };
}

#endif
