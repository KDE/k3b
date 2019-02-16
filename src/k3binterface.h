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


#ifndef _K3B_INTERFACE_H_
#define _K3B_INTERFACE_H_

#include <QObject>
#include <QString>
#include <QStringList>


namespace K3b {
    class MainWindow;

    class Interface : public QObject
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.k3b.MainWindow" )

    public:
        explicit Interface( MainWindow* main );
        ~Interface() override;

    public Q_SLOTS:
        /**
        * Returns a D-BUS path to a newly created project
        */
        QString createDataProject();
        QString createAudioProject();
        QString createMixedProject();
        QString createVcdProject();
        QString createMovixProject();
        QString createVideoDvdProject();

        /**
        * Returns a D-BUS path to the currently active project.
        */
        QString currentProject();

        QString openProject( const QString& url );

        QStringList projects();

        void copyMedium();
        void copyMedium( const QString& dev );
        void formatMedium();
        void writeImage();
        void writeImage( const QString& url );

        void audioCdRip();
        void audioCdRip( const QString& dev );
        void videoCdRip();
        void videoCdRip( const QString& dev );
        void videoDvdRip();
        void videoDvdRip( const QString& dev );

        /**
        * Add URLs to the current active project.
        * If no project is open a new Audio or Data CD
        * project will be created depending on the type
        * of the first file.
        */
        void addUrls( const QStringList& urls );
        void addUrl( const QString& url );

        /**
        * @return true if currently some job is running.
        */
        bool blocked() const;

    private:
        MainWindow* m_main;
    };
}

#endif
