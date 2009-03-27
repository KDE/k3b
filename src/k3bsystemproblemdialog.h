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


#ifndef _K3B_SYSTEM_DIALOG_H_
#define _K3B_SYSTEM_DIALOG_H_

#include <qstring.h>
#include <q3valuelist.h>
//Added by qt3to4:
#include <QCloseEvent>

#include <kdialog.h>

class QPushButton;
class QCheckBox;
class QCloseEvent;

namespace K3b {
    namespace Device {
        class Device;
    }

    class SystemProblem
    {
    public:
        SystemProblem( int type = NON_CRITICAL,
                       const QString& problem = QString(),
                       const QString& details = QString(),
                       const QString& solution = QString(),
                       bool k3bsetup = false );

        enum {
            CRITICAL,
            NON_CRITICAL,
            WARNING
        };

        int type;
        QString problem;
        QString details;
        QString solution;
        bool solvableByK3bSetup;
    };


    /**
     * The SystemProblem checks for problems with the system setup
     * that could prevent K3b from funcioning properly. Examples are
     * missing external appplications like cdrecord or versions of
     * external applications that are too old.
     *
     * Usage:
     * <pre>
     * if( SystemProblemDialog::readCheckSystemConfig() )
     *    SystemProblemDialog::checkSystem( this );
     * </pre>
     */
    class SystemProblemDialog : public KDialog
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
        static void checkSystem( QWidget* parent = 0, NotificationLevel level = NotifyOnlyErrors );

    protected:
        void closeEvent( QCloseEvent* );

    private Q_SLOTS:
        void slotK3bSetup();

    private:
        SystemProblemDialog( const QList<SystemProblem>&,
                             QWidget* parent = 0);
        static int dmaActivated( Device::Device* );
#ifndef Q_OS_WIN32
        static QList<Device::Device*> checkForAutomounting();
#endif
        QPushButton* m_closeButton;
        QPushButton* m_k3bsetupButton;
        QCheckBox* m_checkDontShowAgain;
    };
}

#endif
