/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/



#ifndef K3B_STATUSBAR_MANAGER_H
#define K3B_STATUSBAR_MANAGER_H

#include <QObject>

class QLabel;
class QEvent;
class QTimer;

namespace K3b {
    
    class Doc;
    class MainWindow;
    
    class StatusBarManager : public QObject
    {
        Q_OBJECT

    public:
        explicit StatusBarManager( MainWindow* parent );
        ~StatusBarManager() override;

    public Q_SLOTS:
        void update();

    private Q_SLOTS:
        void showActionStatusText( const QString& text );
        void clearActionStatusText();
        void slotActiveProjectChanged( K3b::Doc* doc );
        void slotUpdateProjectStats();

    private:
        bool eventFilter( QObject* o, QEvent* e ) override;

        QLabel* m_labelInfoMessage;
        QLabel* m_pixFreeTemp;
        QLabel* m_labelFreeTemp;
        QLabel* m_versionBox;
        QLabel* m_labelProjectInfo;

        MainWindow* m_mainWindow;

        QTimer* m_updateTimer;
    };
}

#endif
