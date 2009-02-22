/*
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

#ifndef _K3B_AUDIO_METAINFO_RENAMER_PLUGIN_H_
#define _K3B_AUDIO_METAINFO_RENAMER_PLUGIN_H_


#include <k3bprojectplugin.h>
#include <qwidget.h>

namespace K3b {
    class DirItem;
    class FileItem;
}

class QTreeWidgetItem;


class K3bAudioMetainfoRenamerPluginWidget : public QWidget, public K3b::ProjectPluginGUIBase
{
    Q_OBJECT

public:
    K3bAudioMetainfoRenamerPluginWidget( K3b::Doc* doc, QWidget* parent = 0 );
    ~K3bAudioMetainfoRenamerPluginWidget();

    QWidget* qWidget() { return this; }

    QString title() const;
    QString subTitle() const;

    void loadDefaults();
    void readSettings( const KConfigGroup& );
    void saveSettings( KConfigGroup );

    void activate();

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
    ~K3bAudioMetainfoRenamerPlugin();

    int pluginSystemVersion() const { return K3B_PLUGIN_SYSTEM_VERSION; }

    K3b::ProjectPluginGUIBase* createGUI( K3b::Doc*, QWidget* = 0 );
};


#endif
