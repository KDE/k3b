/*
    SPDX-FileCopyrightText: 2009-2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3BPROJECTTABWIDGET_H
#define K3BPROJECTTABWIDGET_H

#include <QTabWidget>

class QAction;
class KActionMenu;
namespace K3b {
    class Doc;
}


/**
 * An enhanced Tab Widget that shows a context menu fpr K3b projects.
 *
 * @author Sebastian Trueg
 */
namespace K3b {
    class ProjectTabWidget : public QTabWidget
    {
        Q_OBJECT

    public:
        explicit ProjectTabWidget( QWidget *parent = 0 );
        ~ProjectTabWidget() override;

        void addTab( Doc* doc );
        void removeTab( Doc* doc );
        void setCurrentTab( Doc* doc );
        Doc* currentTab() const;

        /**
         * adds the given action into the popup menu for the tabs
         */
        void addAction( QAction* action );
        
    Q_SIGNALS:
        void tabCloseRequested( Doc* doc );
        
    protected:
        /**
         * \return the project for the tab at position \p pos or 0 in case the tab is
         * not a project tab.
         */
        Doc* projectAt( const QPoint& pos ) const;

        bool eventFilter( QObject* o, QEvent* e ) override;

    private Q_SLOTS:
        void slotDocChanged( K3b::Doc* );
        void slotDocSaved( K3b::Doc* );
        void slotTabCloseRequested( int index );

    private:
        class Private;
        Private* d;
    };
}

#endif
