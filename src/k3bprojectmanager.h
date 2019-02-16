/*
 *
 * $Id$
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

#ifndef _K3B_PROJECT_MANAGER_H_
#define _K3B_PROJECT_MANAGER_H_

#include "k3bdoc.h"

#include <QList>
#include <QObject>


class QUrl;

namespace K3b {

    class ProjectManager : public QObject
    {
        Q_OBJECT

    public:
        explicit ProjectManager( QObject* parent = 0 );
        ~ProjectManager() override;

        QList<Doc*> projects() const;

        /**
         * Create a new project including loading user defaults and creating
         * the dcop interface.
         */
        Doc* createProject( Doc::Type type );

        /**
         * Opens a K3b project.
         * \return 0 if url does not point to a valid k3b project file, the new project otherwise.
         */
        Doc* openProject( const QUrl &url );

        /**
         * saves the document under filename and format.
         */
        bool saveProject( Doc*, const QUrl &url );

        Doc* activeDoc() const { return activeProject(); }
        Doc* activeProject() const;
        Doc* findByUrl( const QUrl& url );
        bool isEmpty() const;

        /**
         * \return D-BUS object path of given project
         */
        QString dbusPath( Doc* doc ) const;

    public Q_SLOTS:
        void addProject( K3b::Doc* );
        void removeProject( K3b::Doc* );
        void setActive( K3b::Doc* );
        void loadDefaults( K3b::Doc* );

    Q_SIGNALS:
        void newProject( K3b::Doc* );
        void projectSaved( K3b::Doc* );
        void closingProject( K3b::Doc* );
        void projectChanged( K3b::Doc* doc );
        void activeProjectChanged( K3b::Doc* );

    private Q_SLOTS:
        void slotProjectChanged( K3b::Doc* doc );

    private:
        // used internal
        Doc* createEmptyProject( Doc::Type );

        class Private;
        Private* d;
    };
}

#endif
