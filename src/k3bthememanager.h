/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_THEME_MANAGER_H_
#define _K3B_THEME_MANAGER_H_

#include <QObject>
#include <QString>
#include <QMap>
#include <QColor>
#include <QPixmap>

#include <KConfigGroup>


namespace K3b {
    class Theme
    {
    public:
        Theme();
        explicit Theme( QString name );

        QColor backgroundColor() const;
        QColor foregroundColor() const;

        enum PixmapType {
            MEDIA_AUDIO,      /**< Media information header, right side when showing an audio CD. */
            MEDIA_DATA,       /**< Media information header, right side when showing a data media. */
            MEDIA_VIDEO,      /**< Media information header, right side when showing a video media. */
            MEDIA_EMPTY,      /**< Media information header, right side when showing an empty media. */
            MEDIA_MIXED,      /**< Media information header, right side when showing a mixed mode CD. */
            MEDIA_NONE,       /**< Media information header, right side default pixmap (no media). */
            MEDIA_LEFT,       /**< Media information header, left side. */
            PROGRESS_WORKING, /**< Progress dialog, left top while working. */
            PROGRESS_SUCCESS, /**< Progress dialog, left top on success. */
            PROGRESS_FAIL,    /**< Progress dialog, left top on failure. */
            PROGRESS_RIGHT,   /**< Progress dialog, right top. */
            DIALOG_LEFT,      /**< Action dialog, left top. */
            DIALOG_RIGHT,     /**< Action dialog, right top. */
            SPLASH,           /**< K3b splash screen. Size not important. */
            PROJECT_LEFT,     /**< Project header left side. */
            PROJECT_RIGHT,    /**< Project header right side. */
            WELCOME_BG        /**< Background pixmap of the welcome window. */
        };

        enum BackgroundMode {
            BG_TILE,         /**< Keep the pixmap's size and tile the welcome widget */
            BG_SCALE         /**< Scale the pixmap to fill the welcome widget. */
        };

        QPixmap pixmap( PixmapType ) const;

        /**
         * \deprecated use pixmap( PixmapType )
         */
        QPixmap pixmap( const QString& name ) const;

        BackgroundMode backgroundMode() const;

        QString name() const { return m_name; }
        QString author() const { return m_author; }
        QString comment() const { return m_comment; }
        QString version() const { return m_version; }

        /**
         * Global themes are installed for all users and cannot be deleted.
         */
        bool global() const { return !local(); }

        /**
         * Local themes are installed in the user's home directory and can be deleted.
         */
        bool local() const { return m_local; }

        QString path() const { return m_path; }

        QPalette palette() const;

        static QString filenameForPixmapType( PixmapType );

    private:
        QString m_path;
        bool m_local;
        QString m_name;
        QString m_author;
        QString m_comment;
        QString m_version;
        QColor m_bgColor;
        QColor m_fgColor;
        BackgroundMode m_bgMode;

        mutable QMap<QString, QPixmap> m_pixmapMap;

        QPixmap m_emptyPixmap;

        friend class ThemeManager;
    };


    class ThemeManager : public QObject
    {
        Q_OBJECT

    public:
        explicit ThemeManager( QObject* parent = 0 );
        ~ThemeManager() override;

        QList<Theme*>& themes() const;

        /**
         * This is never null. If no theme could be found an empty dummy theme
         * will be returnes which does not contains any pixmaps.
         */
        Theme* currentTheme() const;
        Theme* findTheme( const QString& ) const;

    Q_SIGNALS:
        void themeChanged();
        void themeChanged( Theme* );

    public Q_SLOTS:
        void readConfig( const KConfigGroup& );
        void saveConfig( KConfigGroup );
        void setCurrentTheme( const QString& );
        void setCurrentTheme( Theme* );
        void loadThemes();

    private:
        void loadTheme( const QString& name );

        class Private;
        Private* d;
    };
}

#endif
