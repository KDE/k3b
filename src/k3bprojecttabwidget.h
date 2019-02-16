/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
