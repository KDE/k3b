/*
    SPDX-FileCopyrightText: 2010-2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_EXTERNAL_BIN_WIDGET_H
#define K3B_EXTERNAL_BIN_WIDGET_H


#include <KAuth/Action>
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

        /**
	     * Decode a @c KAuth::Action::AuthStatus into a textual error description.
	     *
	     * @param status The status from @c KAuth::Action::status()
	     * @return the error description
	     */
        static QString authErrorString(KAuth::Action::AuthStatus status);

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
