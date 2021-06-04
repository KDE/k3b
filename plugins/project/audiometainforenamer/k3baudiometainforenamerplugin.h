/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef _K3B_AUDIO_METAINFO_RENAMER_PLUGIN_H_
#define _K3B_AUDIO_METAINFO_RENAMER_PLUGIN_H_


#include "k3bprojectplugin.h"
#include <QWidget>

namespace K3b {
    class DataDoc;
    class DirItem;
    class FileItem;
}

class QTreeWidgetItem;


class K3bAudioMetainfoRenamerPluginWidget : public QWidget, public K3b::ProjectPluginGUIBase
{
    Q_OBJECT

public:
    explicit K3bAudioMetainfoRenamerPluginWidget( K3b::DataDoc* doc, QWidget* parent = 0 );
    ~K3bAudioMetainfoRenamerPluginWidget() override;

    QWidget* qWidget() override { return this; }

    QString title() const override;
    QString subTitle() const override;

    void readSettings( const KConfigGroup& ) override;
    void saveSettings( KConfigGroup ) override;

    void activate() override;

private Q_SLOTS:
    void slotScanClicked();

private:
    void scanDir( K3b::DirItem*, QTreeWidgetItem* parent );
    QString createNewName( K3b::FileItem* );
    bool existsOtherItemWithSameName( K3b::FileItem*, const QString& );

    class Private;
    Private* d;
};


class K3bAudioMetainfoRenamerPlugin : public K3b::ProjectPlugin
{
    Q_OBJECT

public:
    K3bAudioMetainfoRenamerPlugin( QObject* parent, const QVariantList& );
    ~K3bAudioMetainfoRenamerPlugin() override;

    int pluginSystemVersion() const override { return K3B_PLUGIN_SYSTEM_VERSION; }

    K3b::ProjectPluginGUIBase* createGUI( K3b::Doc*, QWidget* = 0 ) override;
};


#endif
