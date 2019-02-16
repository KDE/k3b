/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BVIEW_H
#define K3BVIEW_H

#include "k3bdoc.h"

#ifdef HAVE_CONFIG_H
#include <config-k3b.h>
#endif

#include <KXMLGUIClient>

#include <QHash>
#include <QList>
#include <QUrl>
#include <QWidget>

class QVBoxLayout;
class KToolBar;

namespace K3b {
    class FillStatusDisplay;
    class ProjectBurnDialog;
    class ProjectPlugin;

    class View : public QWidget, public KXMLGUIClient
    {
        Q_OBJECT

    public:
        /**
         *
         */
        View( Doc* pDoc, QWidget* parent );
        ~View() override;

        /**
         * returns a pointer to the document connected to the view
         * @deprecated use doc()
         */
        Doc* getDocument() const { return m_doc; }
        Doc* doc() const { return m_doc; }

        void setMainWidget( QWidget* );

    public Q_SLOTS:
        /**
         * Default impl. brings up the burnDialog via newBurnDialog() with writing
         */
        virtual void slotBurn();

        /**
         * Default impl. brings up the burnDialog via newBurnDialog() without writing
         */
        virtual void slotProperties();

        /**
         * Add an url to the doc. The default implementation simply calls
         * addUrls.
         */
        virtual void addUrl( const QUrl& );

        /**
         * Add urls to the doc. The default implementation calls doc()->addUrls.
         */
        virtual void addUrls( const QList<QUrl>& );

    protected:
        /**
         * Protected since the BurnDialog is not part of the API.
         */
        virtual ProjectBurnDialog* newBurnDialog( QWidget* = 0) = 0;

        /**
         * Call this to get the projectplugin actions.
         * It is usually used to place plugins buttons on the toolbar.
         */
        QList<QAction*> createPluginsActions( Doc::Type docType );

        FillStatusDisplay* fillStatusDisplay() const { return m_fillStatusDisplay; }
        KToolBar* toolBox() const { return m_toolBox; }

    private Q_SLOTS:
        void slotPluginButtonClicked();

    private:
        Doc* m_doc;
        FillStatusDisplay* m_fillStatusDisplay;
        KToolBar* m_toolBox;
        QVBoxLayout* m_layout;

        QHash<const QObject*, K3b::ProjectPlugin*> m_plugins;
    };
}

#endif // K3BVIEW_H
