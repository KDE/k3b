/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_SYSTEM_PROBLEM_DIALOG_H_
#define _K3B_SYSTEM_PROBLEM_DIALOG_H_

#include <QString>
#include <QDialog>

class QCheckBox;
class QCloseEvent;

namespace K3b {
    namespace Device {
        class Device;
    }

    class SystemProblem
    {
    public:
        enum Type
        {
            CRITICAL,
            NON_CRITICAL,
            WARNING
        };
        
        explicit SystemProblem( Type type = NON_CRITICAL,
                       const QString& problem = QString(),
                       const QString& details = QString(),
                       const QString& solution = QString() );

        Type type;
        QString problem;
        QString details;
        QString solution;
    };


    /**
     * The SystemProblem checks for problems with the system setup
     * that could prevent K3b from funcioning properly. Examples are
     * missing external applications like cdrecord or versions of
     * external applications that are too old.
     *
     * Usage:
     * <pre>
     * if( SystemProblemDialog::readCheckSystemConfig() )
     *    SystemProblemDialog::checkSystem( this );
     * </pre>
     */
    class SystemProblemDialog : public QDialog
    {
        Q_OBJECT

    public:
        enum NotificationLevel {
            AlwaysNotify,
            NotifyOnlyErrors
        };

        /**
         * Determines if the system problem dialog should be shown or not.
         * It basicaly reads a config entry. But in addition it
         * always forces the system check if a new version has been installed
         * or K3b is started for the first time.
         */
        static bool readCheckSystemConfig();
        static void checkSystem(QWidget* parent = 0, NotificationLevel level = NotifyOnlyErrors, bool forceCheck = false);

    protected:
        void done(int) override;

    private Q_SLOTS:
        void slotShowDeviceSettings();
        void slotShowBinSettings();

    private:
        SystemProblemDialog( const QList<SystemProblem>& problems,
                             bool showDeviceSettingsButton,
                             bool showBinSettingsButton,
                             bool forceCheck = false,
                             QWidget* parent = 0);
        static int dmaActivated( Device::Device* );
#ifndef Q_OS_WIN32
        static QList<Device::Device*> checkForAutomounting();
#endif
        QCheckBox* m_checkDontShowAgain;
    };
}

#endif
