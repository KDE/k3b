/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010-2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3B_EXTERNAL_BIN_WIDGET_H
#define K3B_EXTERNAL_BIN_WIDGET_H


#include "config-k3b.h"
#include <QWidget>


class QModelIndex;
class QPushButton;
class QTabWidget;
class QTreeView;
class KEditListWidget;

namespace K3b {
    class ExternalBinManager;
    class ExternalProgram;
    class ExternalBin;
    class ExternalBinModel;
    class ExternalBinParamsModel;
    class ExternalBinPermissionModel;

    class ExternalBinWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit ExternalBinWidget( ExternalBinManager* manager, QWidget* parent = 0 );
        ~ExternalBinWidget() override;

    public Q_SLOTS:
        void rescan();
        void load();
        void save();

    private Q_SLOTS:
        void saveSearchPath();
        void slotPermissionModelChanged();
        void slotChangePermissions();

    private:
        ExternalBinManager* m_manager;
        ExternalBinModel* m_programModel;
        ExternalBinParamsModel* m_parameterModel;
        ExternalBinPermissionModel* m_permissionModel;

        QTabWidget* m_mainTabWidget;
        QTreeView* m_programView;
        QTreeView* m_parameterView;
        QTreeView* m_permissionView;
        KEditListWidget* m_searchPathBox;

        QPushButton* m_changePermissionsButton;
        QPushButton* m_rescanButton;
    };
}


#endif
