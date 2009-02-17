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

#include <qobject.h>
#include <qlist.h>
//Added by qt3to4:
#include <QCustomEvent>

#include "k3b_export.h"
#include "config-k3b.h"

#define LIBK3B_VERSION K3B_VERSION_STRING

#define k3bcore K3bCore::k3bCore()


class K3bExternalBinManager;
class K3bVersion;
class KConfig;
class K3bJob;
class K3bBurnJob;
class K3bGlobalSettings;
class K3bPluginManager;
class QCustomEvent;
class K3bMediaCache;


namespace K3bDevice {
    class DeviceManager;
    class Device;
}


/**
 * The K3b core takes care of the managers. 
 * This has been separated from K3bApplication to 
 * make creating a K3bPart easy.
 * This is the heart of the K3b system. Every plugin may use this
 * to get the information it needs.
 */
class LIBK3B_EXPORT K3bCore : public QObject
{
    Q_OBJECT

public:
    /**
     * Although K3bCore is a singlelton it's constructor is not private to make inheritance
     * possible. Just make sure to only create one instance.
     */
    K3bCore( QObject* parent = 0 );
    virtual ~K3bCore();

    QList<K3bJob*> runningJobs() const;

    /**
     * Equals to !runningJobs().isEmpty()
     */
    bool jobsRunning() const;

    /**
     * The default implementation calls add four initXXX() methods,
     * scans for devices, applications, and reads the global settings.
     */
    virtual void init();

    /**
     * @param c if 0 K3bCore uses the K3b configuration
     */
    virtual void readSettings( KConfig* c = 0 );

    /**
     * @param c if 0 K3bCore uses the K3b configuration
     */
    virtual void saveSettings( KConfig* c = 0 );

    K3bMediaCache* mediaCache() const;

    /**
     * If this is reimplemented it is recommended to also reimplement
     * init().
     */
    virtual K3bDevice::DeviceManager* deviceManager() const;

    /**
     * Returns the external bin manager from K3bCore.
     *
     * By default K3bCore only adds the default programs:
     * cdrecord, cdrdao, growisofs, mkisofs, dvd+rw-format, readcd
     *
     * If you need other programs you have to add them manually like this:
     * <pre>externalBinManager()->addProgram( new K3bNormalizeProgram() );</pre>
     */
    K3bExternalBinManager* externalBinManager() const;
    K3bPluginManager* pluginManager() const;

    /**
     * Global settings used throughout libk3b. Change the settings directly in the
     * K3bGlobalSettings object. They will be saved by K3bCore::saveSettings
     */
    K3bGlobalSettings* globalSettings() const;

    /**
     * returns the version of the library as defined by LIBK3B_VERSION
     */
    const K3bVersion& version() const;

    /**
     * Default implementation returns the K3b configuration from k3brc.
     * Normally this should not be used.
     */
    virtual KConfig* config() const;

    /**
     * Used by the writing jobs to block a device.
     * This makes sure no device is used twice within libk3b
     *
     * When using this method in a job be aware that reimplementations might
     * open dialogs and resulting in a blocking call.
     *
     * This method calls internalBlockDevice() to do the actual work.
     */
    bool blockDevice( K3bDevice::Device* );
    void unblockDevice( K3bDevice::Device* );

    static K3bCore* k3bCore() { return s_k3bCore; }

Q_SIGNALS:
    /**
     * Emitted once a new job has been started. This includes burn jobs.
     */
    void jobStarted( K3bJob* );
    void burnJobStarted( K3bBurnJob* );
    void jobFinished( K3bJob* );
    void burnJobFinished( K3bBurnJob* );

public Q_SLOTS:
    /**
     * Every running job registers itself with the core.
     * For now this is only used to determine if some job
     * is running.
     */
    void registerJob( K3bJob* job );
    void unregisterJob( K3bJob* job );

protected:
    /**
     * Reimplement this to add additional checks.
     *
     * This method is thread safe. blockDevice makes sure
     * it is only executed in the GUI thread.
     */
    virtual bool internalBlockDevice( K3bDevice::Device* );
    virtual void internalUnblockDevice( K3bDevice::Device* );

    virtual void initGlobalSettings();
    virtual void initExternalBinManager();
    virtual void initDeviceManager();
    virtual void initPluginManager();
    virtual void initMediaCache();

    virtual void customEvent( QEvent* e );

private:
    class Private;
    Private* d;

    static K3bCore* s_k3bCore;
};

#endif
