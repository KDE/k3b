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


class K3bDirItem;
class K3bFileItem;
class QTreeWidgetItem;


class K3bAudioMetainfoRenamerPluginWidget : public QWidget, public K3bProjectPluginGUIBase
{
    Q_OBJECT

public:
    K3bAudioMetainfoRenamerPluginWidget( K3bDoc* doc, QWidget* parent = 0 );
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
    void scanDir( K3bDirItem*, QTreeWidgetItem* parent );
    QString createNewName( K3bFileItem* );
    bool existsOtherItemWithSameName( K3bFileItem*, const QString& );

    class Private;
    Private* d;
};


class K3bAudioMetainfoRenamerPlugin : public K3bProjectPlugin
{
    Q_OBJECT

public:
    K3bAudioMetainfoRenamerPlugin( QObject* parent, const QVariantList& );
    ~K3bAudioMetainfoRenamerPlugin();

    int pluginSystemVersion() const { return K3B_PLUGIN_SYSTEM_VERSION; }

    K3bProjectPluginGUIBase* createGUI( K3bDoc*, QWidget* = 0 );
};


#endif
