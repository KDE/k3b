/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010-2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bcore.h"

#include <QMap>
#include <QScopedPointer>
#include <QApplication>

#define k3bappcore K3b::Application::Core::k3bAppCore()

class QCommandLineParser;

namespace K3b {
    class MainWindow;
    class ThemeManager;
    class ProjectManager;
    class AppDeviceManager;

    class Application : public QApplication
    {
        Q_OBJECT

    public:
        Application( int& argc, char** argv );
        ~Application() override;

        void init( QCommandLineParser* commandLineParser );

        class Core;

    private Q_SLOTS:
        void slotShutDown();

    private:
        Q_INVOKABLE void checkSystemConfig();
        Q_INVOKABLE void processCmdLineArgs();

        QScopedPointer<QCommandLineParser> m_cmdLine;
        Core* m_core;
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
        ~Core() override;

        Q_INVOKABLE void init() override;

        Q_INVOKABLE void readSettings( KSharedConfig::Ptr c ) override;
        Q_INVOKABLE void saveSettings( KSharedConfig::Ptr c ) override;

        AppDeviceManager* appDeviceManager() const;

        ThemeManager* themeManager() const { return m_themeManager; }

        ProjectManager* projectManager() const { return m_projectManager; }

        MainWindow* k3bMainWindow() const { return m_mainWindow; }

        static Core* k3bAppCore() { return s_k3bAppCore; }

    Q_SIGNALS:
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
        Device::DeviceManager* createDeviceManager() const override;

        bool internalBlockDevice( Device::Device* ) override;
        void internalUnblockDevice( Device::Device* ) override;

        ThemeManager* m_themeManager;
        MainWindow* m_mainWindow;
        ProjectManager* m_projectManager;

        QMap<Device::Device*, int> m_deviceBlockMap;

        static Core* s_k3bAppCore;

        friend class K3b::Application;
    };
}

#endif
