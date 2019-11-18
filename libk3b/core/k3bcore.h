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


#ifndef _K3B_CORE_H_
#define _K3B_CORE_H_

#include "k3b_export.h"
#include "config-k3b.h"

#include <KSharedConfig>

#include <QObject>
#include <QList>

#define LIBK3B_VERSION K3B_VERSION_STRING

#define k3bcore K3b::Core::k3bCore()


class KConfig;

namespace K3b {

    class ExternalBinManager;
    class Version;
    class Job;
    class BurnJob;
    class GlobalSettings;
    class PluginManager;
    class MediaCache;

    namespace Device {
        class DeviceManager;
        class Device;
    }

    /**
     * The K3b core takes care of the managers.
     * This has been separated from Application to
     * make creating a Part easy.
     * This is the heart of the K3b system. Every plugin may use this
     * to get the information it needs.
     */
    class LIBK3B_EXPORT Core : public QObject
    {
        Q_OBJECT

    public:
        /**
         * Although Core is a singleton it's constructor is not private to make inheritance
         * possible. Just make sure to only create one instance.
         */
        explicit Core( QObject* parent = 0 );
        ~Core() override;

        QList<Job*> runningJobs() const;

        /**
         * Equals to !runningJobs().isEmpty()
         */
        bool jobsRunning() const;

        /**
         * The default implementation calls add four initXXX() methods,
         * scans for devices, applications, and reads the global settings.
         */
        virtual void init();

        virtual void readSettings( KSharedConfig::Ptr c );

        virtual void saveSettings( KSharedConfig::Ptr c );

        MediaCache* mediaCache() const;

        Device::DeviceManager* deviceManager() const;

        /**
         * Returns the external bin manager from Core.
         *
         * By default Core only adds the default programs:
         * cdrecord, cdrdao, growisofs, mkisofs, dvd+rw-format, readcd
         *
         * If you need other programs you have to add them manually like this:
         * <pre>externalBinManager()->addProgram( new NormalizeProgram() );</pre>
         */
        ExternalBinManager* externalBinManager() const;
        PluginManager* pluginManager() const;

        /**
         * Global settings used throughout libk3b. Change the settings directly in the
         * GlobalSettings object. They will be saved by Core::saveSettings
         */
        GlobalSettings* globalSettings() const;

        /**
         * returns the version of the library as defined by LIBK3B_VERSION
         */
        Version version() const;

        /**
         * Used by the writing jobs to block a device.
         * This makes sure no device is used twice within libk3b
         *
         * When using this method in a job be aware that reimplementations might
         * open dialogs and resulting in a blocking call.
         *
         * This method calls internalBlockDevice() to do the actual work.
         */
        bool blockDevice( Device::Device* );
        void unblockDevice( Device::Device* );

        /**
         * \return \p true if \p dev has been blocked via blockDevice.
         */
        bool deviceBlocked( Device::Device* dev ) const;

        static Core* k3bCore() { return s_k3bCore; }

    Q_SIGNALS:
        /**
         * Emitted once a new job has been started. This includes burn jobs.
         */
        void jobStarted( K3b::Job* );
        void burnJobStarted( K3b::BurnJob* );
        void jobFinished( K3b::Job* );
        void burnJobFinished( K3b::BurnJob* );

    public Q_SLOTS:
        /**
         * Every running job registers itself with the core.
         * For now this is only used to determine if some job
         * is running.
         */
        void registerJob( K3b::Job* job );
        void unregisterJob( K3b::Job* job );

    protected:
        /**
         * Reimplement this to add additional checks.
         *
         * This method is thread safe. blockDevice makes sure
         * it is only executed in the GUI thread.
         */
        virtual bool internalBlockDevice( Device::Device* );
        virtual void internalUnblockDevice( Device::Device* );

        virtual Device::DeviceManager* createDeviceManager() const;

        void customEvent( QEvent* e ) override;

    private:
        class Private;
        Private* d;

        static Core* s_k3bCore;
    };
}

#endif
