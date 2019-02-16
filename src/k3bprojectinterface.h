/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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


#ifndef _K3B_PROJECT_INTERFACE_H_
#define _K3B_PROJECT_INTERFACE_H_

#include <KIO/Global>
#include <QObject>
#include <QString>
#include <QStringList>

/**
 * Base class for all project interfaces
 */
namespace K3b {
    class Doc;

    class ProjectInterface : public QObject
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.k3b.Project" )

    public:
        explicit ProjectInterface( Doc* doc, const QString& dbusPath = QString() );
        ~ProjectInterface() override;

        QString dbusPath() const;

    public Q_SLOTS:
        void addUrls( const QStringList& urls );
        void addUrl( const QString& url );

        /**
        * Opens the burn dialog
        */
        void burn();

        /**
        * Starts the burning immedeately
        * \return true if the burning could be started. Be aware that the return
        *         value does not say anything about the success of the burning
        *         process.
        */
        bool directBurn();

        void setBurnDevice( const QString& blockdevicename );

        /**
        * \return the length of the project in blocks (frames).
        */
        int length() const;

        /**
        * \return size of the project in bytes.
        */
        KIO::filesize_t size() const;

        const QString& imagePath() const;

        /**
        * \return A string representation of the project type. One of:
        * \li "data" - Data
        * \li "audiocd" - Audio CD
        * \li "mixedcd" - Mixed Mode CD
        * \li "videocd" - Video CD
        * \li "emovix" - eMovix
        * \li "videodvd" - Video DVD
        *
        * Be aware that this is not the same as Doc::documentType for historical reasons.
        */
        QString projectType() const;

    private:
        Doc* m_doc;
        QString m_dbusPath;
    };
}

#endif
