/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include <qtabwidget.h>
//Added by qt3to4:
#include <QEvent>
#include <kurl.h>

class KAction;
class KActionMenu;
namespace K3b {
    class Doc;
}


/**
 * An enhanced Tab Widget that hides the tabbar in case only one page has been inserted
 * and shows a context menu fpr K3b projects.
 *
 * @author Sebastian Trueg
 */
namespace K3b {
    class ProjectTabWidget : public QTabWidget
    {
        Q_OBJECT

    public:
        ProjectTabWidget( QWidget *parent = 0 );
        ~ProjectTabWidget();

        void insertTab( Doc* );

        void addTab( QWidget * child, const QString & label );
        void addTab( QWidget * child, const QIcon & iconset, const QString & label );
        void insertTab( QWidget * child, const QString & label, int index = -1 );
        void insertTab( QWidget * child, const QIcon & iconset, const QString & label, int index = -1 );

        /**
         * \return the project for the tab at position \p pos or 0 in case the tab is
         * not a project tab.
         */
        Doc* projectAt( const QPoint& pos ) const;

        /**
         * inserts the given action into the popup menu for the tabs
         */
        void insertAction( KAction* );

        bool eventFilter( QObject* o, QEvent* e );

    protected:
        virtual void tabInserted ( int index );

    public Q_SLOTS:
        void removePage( QWidget* );

    private Q_SLOTS:
        void slotDocChanged( K3b::Doc* );
        void slotDocSaved( K3b::Doc* );

    private:
        KActionMenu* m_projectActionMenu;

        class ProjectData;
        QMap<Doc*, ProjectData> m_projectDataMap;
    };
}

#endif
