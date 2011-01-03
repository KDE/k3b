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


#ifndef _K3B_APPLICATION_H_
#define _K3B_APPLICATION_H_

#include <kuniqueapplication.h>
#include "k3bcore.h"

#include <qmap.h>

#define k3bappcore K3b::Application::Core::k3bAppCore()


namespace K3b {
    class MainWindow;
    class AudioServer;
    class ThemeManager;
    class ProjectManager;
    class AppDeviceManager;


    class Application : public KUniqueApplication
    {
        Q_OBJECT

    public:
        Application();
        ~Application();

        int newInstance();

        class Core;

    public Q_SLOTS:
        void init();

    Q_SIGNALS:
        void initializationInfo( const QString& );
        void initializationDone();

    private Q_SLOTS:
        void slotShutDown();

    private:
        bool processCmdLineArgs();

        Core* m_core;
        //AudioServer* m_audioServer;
        MainWindow* m_mainWindow;
    };


    /**
     * The application's core which extends Core with some additional features
     * like the thememanager or an enhanced device manager.
     */
    class Application::Core : public K3b::Core
    {
        Q_OBJECT

    public:
        Core( QObject* parent );
        ~Core();

        void init();

        void readSettings( KSharedConfig::Ptr c );
        void saveSettings( KSharedConfig::Ptr c );

        /**
         * \reimplemented from Core. We use our own devicemanager here.
         */
        Device::DeviceManager* deviceManager() const;

        AppDeviceManager* appDeviceManager() const { return m_appDeviceManager; }

        ThemeManager* themeManager() const { return m_themeManager; }

        ProjectManager* projectManager() const { return m_projectManager; }

        MainWindow* k3bMainWindow() const { return m_mainWindow; }

        static Core* k3bAppCore() { return s_k3bAppCore; }

    Q_SIGNALS:
        /**
         * This is used for showing info in the K3b splashscreen
         */
        void initializationInfo( const QString& );

        /**
         * Any component may request busy info
         * In the K3b main app this will be displayed
         * as a moving square in the taskbar
         *
         * FIXME: this is bad design
         */
        void busyInfoRequested( const QString& );

        /**
         * FIXME: this is bad design
         */
        void busyFinishRequested();

    private:
        void initDeviceManager();

        bool internalBlockDevice( Device::Device* );
        void internalUnblockDevice( Device::Device* );

        ThemeManager* m_themeManager;
        MainWindow* m_mainWindow;
        ProjectManager* m_projectManager;
        AppDeviceManager* m_appDeviceManager;

        QMap<Device::Device*, int> m_deviceBlockMap;

        static Core* s_k3bAppCore;

        friend class K3b::Application;
    };
}

#endif
