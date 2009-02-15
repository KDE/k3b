/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BOPTIONDIALOG_H
#define K3BOPTIONDIALOG_H

#include <KPageDialog>
#include <QHash>

class KPageWidgetItem;
class K3bMiscOptionTab;
class K3bDeviceOptionTab;
class K3bExternalBinOptionTab;
class K3bNotifyOptionTab;
class K3bPluginOptionTab;
class K3bThemeOptionTab;
class K3bCddbOptionTab;
class K3bAdvancedOptionTab;

/**
 *@author Sebastian Trueg
 */
class K3bOptionDialog : public KPageDialog
{
    Q_OBJECT

public:
    K3bOptionDialog( QWidget* parent = 0 );
    ~K3bOptionDialog();

    enum ConfigPage {
        Misc = 0,
        Devices,
        Programs,
        Notifications,
        Plugins,
        Themes,
        Cddb,
        Advanced
    };

public Q_SLOTS:
    void setCurrentPage( ConfigPage page );

protected Q_SLOTS:
    void slotOk();
    void slotApply();
    void slotDefault();

private:
    bool saveSettings();

    // misc options
    K3bMiscOptionTab* m_miscOptionTab;
    KPageWidgetItem* m_miscPage;
    void setupMiscPage();

    // device options
    K3bDeviceOptionTab* m_deviceOptionTab;
    KPageWidgetItem* m_devicePage;
    void setupDevicePage();

    // programs options
    K3bExternalBinOptionTab* m_externalBinOptionTab;
    KPageWidgetItem* m_programsPage;
    void setupProgramsPage();

    // notify options
    K3bNotifyOptionTab* m_notifyOptionTab;
    KPageWidgetItem* m_notifyPage;
    void setupNotifyPage();

    // plugin options
    K3bPluginOptionTab* m_pluginOptionTab;
    KPageWidgetItem* m_pluginPage;
    void setupPluginPage();

    // theme options
    K3bThemeOptionTab* m_themeOptionTab;
    KPageWidgetItem* m_themePage;
    void setupThemePage();

    // cddb options
    K3bCddbOptionTab* m_cddbOptionTab;
    KPageWidgetItem* m_cddbPage;
    void setupCddbPage();

    // advanced options
    K3bAdvancedOptionTab* m_advancedOptionTab;
    KPageWidgetItem* m_advancedPage;
    void setupAdvancedPage();

    typedef QHash<ConfigPage,KPageWidgetItem*> Pages;
    Pages m_pages;
};

#endif
