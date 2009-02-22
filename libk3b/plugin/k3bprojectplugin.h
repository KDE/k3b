/*
 *
 * Copyright (C) 2004-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_PROJECT_PLUGIN_H_
#define _K3B_PROJECT_PLUGIN_H_

#include <k3bplugin.h>

#include "k3b_export.h"

class KConfigGroup;

namespace K3b {
    class Doc;

    /**
     * In case your plugin provides a GUI it is recommended to use the
     * ProjectPluginGUIBase interface. That way K3b can embed the GUI into
     * a fancy dialog which fits the overall look.
     *
     * This is not derived from QWidget to make it possible to inherit
     * from other QWidget derivates.
     */
    class ProjectPluginGUIBase
    {
    public:
        ProjectPluginGUIBase() {}
        virtual ~ProjectPluginGUIBase() {}

        virtual QWidget* qWidget() = 0;

        /**
         * Title used for the GUI
         */
        virtual QString title() const = 0;
        virtual QString subTitle() const { return QString(); }

        virtual void readSettings( const KConfigGroup& ) {}
        virtual void saveSettings( KConfigGroup ) {}

        /**
         * Load system defaults for the GUI
         */
        virtual void loadDefaults() {}

        /**
         * Start the plugin. This method should do the actual work.
         */
        virtual void activate() = 0;
    };


    /**
     * A ProjectPlugin is supposed to modify a k3b project in some way or
     * create additional data based on the project.
     *
     * Reimplement createGUI or activate and use setText, setToolTip, setWhatsThis, and setIcon
     * to specify the gui elements used when presenting the plugin to the user.
     */
    class LIBK3B_EXPORT ProjectPlugin : public Plugin
    {
        Q_OBJECT

    public:
        /**
         * @param type The type of the plugin
         * @param gui If true the plugin is supposed to provide a widget via @p createGUI(). In that case
         *            @p activate() will not be used. A plugin has a GUI if it's functionality is started
         *            by some user input.
         */
        ProjectPlugin( int type, bool gui = false, QObject* parent = 0 );

        virtual ~ProjectPlugin() {
        }

        // TODO: move this to Doc?
        enum Type {
            AUDIO_CD = 0x1,
            DATA_CD = 0x2,
            MIXED_CD = 0x4,
            VIDEO_CD = 0x8,
            MOVIX_CD = 0x10,
            DATA_DVD = 0x20,
            VIDEO_DVD = 0x40,
            MOVIX_DVD = 0x80,
            DATA_PROJECTS = DATA_CD|DATA_DVD,
            MOVIX_PROJECTS = MOVIX_CD|MOVIX_DVD
        };

        // TODO: maybe we should use something like "ProjectPlugin/AudioCD" based on the type?
        QString category() const { return "ProjectPlugin"; }

        QString categoryName() const;

        /**
         * audio, data, videocd, or videodvd
         * Needs to return a proper type. The default implementation returns the type specified
         * in the constructor.
         */
        virtual int type() const { return m_type; }

        /**
         * Text used for menu entries and the like.
         */
        QString text() const { return m_text; }
        QString toolTip() const { return m_toolTip; }
        QString whatsThis() const { return m_whatsThis; }
        QString icon() const { return m_icon; }

        bool hasGUI() const { return m_hasGUI; }

        /**
         * Create the GUI which provides the features for the plugin.
         * This only needs to be implemented in case hasGUI returns true.
         * The returned object has to be a QWidget based class.
         *
         * @param doc based on the type returned by the factory
         *            this will be the doc to work on. It should
         *            be dynamically casted to the needed project type.
         */
        virtual ProjectPluginGUIBase* createGUI( Doc* doc, QWidget* = 0 ) { Q_UNUSED(doc); return 0; }

        /**
         * This is where the action happens.
         * There is no need to implement this in case hasGUI returns true.
         *
         * @param doc based on the type returned by the factory
         *            this will be the doc to work on. It should
         *            be dynamically casted to the needed project type.
         *
         * @param parent the parent widget to be used for things like progress dialogs.
         */
        virtual void activate( Doc* doc, QWidget* parent ) { Q_UNUSED(doc); Q_UNUSED(parent); }

    protected:
        void setText( const QString& s ) { m_text = s; }
        void setToolTip( const QString& s ) { m_toolTip = s; }
        void setWhatsThis( const QString& s ) { m_whatsThis = s; }
        void setIcon( const QString& s ) { m_icon = s; }

    private:
        int m_type;
        bool m_hasGUI;
        QString m_text;
        QString m_toolTip;
        QString m_whatsThis;
        QString m_icon;
    };
}


#endif
