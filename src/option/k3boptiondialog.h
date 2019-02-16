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


#ifndef K3BOPTIONDIALOG_H
#define K3BOPTIONDIALOG_H

#include <KPageDialog>
#include <QHash>

class KPageWidgetItem;
namespace K3b {
    class MiscOptionTab;
}
namespace K3b {
    class DeviceOptionTab;
}
namespace K3b {
    class ExternalBinOptionTab;
}
namespace K3b {
    class NotifyOptionTab;
}
namespace K3b {
    class PluginOptionTab;
}
namespace K3b {
    class ThemeOptionTab;
}
namespace K3b {
    class CddbOptionTab;
}
namespace K3b {
    class AdvancedOptionTab;
}

/**
 *@author Sebastian Trueg
 */
namespace K3b {
class OptionDialog : public KPageDialog
{
    Q_OBJECT

public:
    explicit OptionDialog( QWidget* parent = 0 );
    ~OptionDialog() override;

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
    MiscOptionTab* m_miscOptionTab;
    KPageWidgetItem* m_miscPage;
    void setupMiscPage();

    // device options
    DeviceOptionTab* m_deviceOptionTab;
    KPageWidgetItem* m_devicePage;
    void setupDevicePage();

    // programs options
    ExternalBinOptionTab* m_externalBinOptionTab;
    KPageWidgetItem* m_programsPage;
    void setupProgramsPage();

    // notify options
    NotifyOptionTab* m_notifyOptionTab;
    KPageWidgetItem* m_notifyPage;
    void setupNotifyPage();

    // plugin options
    PluginOptionTab* m_pluginOptionTab;
    KPageWidgetItem* m_pluginPage;
    void setupPluginPage();

    // theme options
    ThemeOptionTab* m_themeOptionTab;
    KPageWidgetItem* m_themePage;
    void setupThemePage();

    // cddb options
    CddbOptionTab* m_cddbOptionTab;
    KPageWidgetItem* m_cddbPage;
    void setupCddbPage();

    // advanced options
    AdvancedOptionTab* m_advancedOptionTab;
    KPageWidgetItem* m_advancedPage;
    void setupAdvancedPage();

    typedef QHash<ConfigPage,KPageWidgetItem*> Pages;
    Pages m_pages;
};
}

#endif
