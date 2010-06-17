/*
 *
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
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



#ifndef K3B_H
#define K3B_H

#include "option/k3boptiondialog.h"

#include <KSharedConfig>
#include <KUrl>
#include <KXmlGuiWindow>

#include <QList>

namespace K3b {
    class Doc;
    class View;
    class ExternalBinManager;

    namespace Device {
        class DeviceManager;
        class Device;
    }

    class MainWindow : public KXmlGuiWindow
    {
        Q_OBJECT

    public:
        /** construtor of MainWindow, calls all init functions to create the application.
         * @see initMenuBar initToolBar
         */
        MainWindow();
        ~MainWindow();

        /** opens a file specified by commandline option */
        Doc* openDocument( const KUrl& url = KUrl() );

        Device::DeviceManager* deviceManager() const;
        ExternalBinManager* externalBinManager() const;
        KSharedConfig::Ptr config() const;

        /**
         * @returns a pointer to the currently visible view or 0 if no project was created
         */
        View* activeView() const;
        /**
         * @returns a pointer to the doc associated with the currently visible view or 0 if no project was created
         */
        Doc* activeDoc() const;

        QList<Doc*> projects() const;

        bool eject();
        void showOptionDialog( OptionDialog::ConfigPage page = OptionDialog::Misc );

        /** Creates the main view of the KDockMainWindow instance and initializes the MDI view area including any needed
         *  connections.
         *  must be called after construction
         */
        void initView();

        /**
         * Reimplemented from QMainWindow, adds "Lock Panels" action
         */
        virtual QMenu* createPopupMenu();

    public Q_SLOTS:
        K3b::Doc* slotNewAudioDoc();
        K3b::Doc* slotNewDataDoc();
        K3b::Doc* slotNewMixedDoc();
        K3b::Doc* slotNewVcdDoc();
        K3b::Doc* slotNewMovixDoc();
        K3b::Doc* slotNewVideoDvdDoc();
        K3b::Doc* slotContinueMultisession();

        void slotClearProject();

        void slotWriteImage();
        void slotWriteImage( const KUrl& url );
        void formatMedium( K3b::Device::Device* );
        void slotFormatMedium();
        void mediaCopy( K3b::Device::Device* );
        void slotMediaCopy();
        void cddaRip( K3b::Device::Device* );
        void slotCddaRip();
        void videoDvdRip( K3b::Device::Device* );
        void slotVideoDvdRip();
        void videoCdRip( K3b::Device::Device* );
        void slotVideoCdRip();
        void slotK3bSetup();

        void showDiskInfo( K3b::Device::Device* );

        void slotErrorMessage(const QString&);
        void slotWarningMessage(const QString&);

        void slotConfigureKeys();
        void slotShowTips();
        void slotCheckSystem();
        void slotManualCheckSystem();

        void addUrls( const KUrl::List& urls );

    Q_SIGNALS:
        void initializationInfo( const QString& );
        void configChanged( KSharedConfig::Ptr c );

    protected:
        /** queryClose is called by KTMainWindow on each closeEvent of a window. Against the
         * default implementation (only returns true), this overridden function retrieves all modified documents
         * from the open document list and asks the user to select which files to save before exiting the application.
         * @see KTMainWindow#queryClose
         * @see KTMainWindow#closeEvent
         */
        virtual bool queryClose();

        /** queryExit is called by KTMainWindow when the last window of the application is going to be closed during the closeEvent().
         * Against the default implementation that just returns true, this calls saveOptions() to save the settings of the last window's
         * properties.
         * @see KTMainWindow#queryExit
         * @see KTMainWindow#closeEvent
         */
        virtual bool queryExit();

        /** saves the window properties for each open window during session end to the session config file, including saving the currently
         * opened file by a temporary filename provided by KApplication.
         * @see KTMainWindow#saveProperties
         */
        virtual void saveProperties(KConfigGroup &_cfg);

        /** reads the session config file and restores the application's state including the last opened files and documents by reading the
         * temporary files saved by saveProperties()
         * @see KTMainWindow#readProperties
         */
        virtual void readProperties(const KConfigGroup &_cfg);

        /**
         * checks if doc is modified and asks the user for saving if so.
         * returns false if the user chose cancel.
         */
        bool canCloseDocument( Doc* );

    private Q_SLOTS:
        /** open a file and load it into the document*/
        void slotFileOpen();
        /** opens a file from the recent files menu */
        void slotFileOpenRecent(const KUrl& url);
        /** save a document */
        void slotFileSave();
        /** save a document by a new filename*/
        void slotFileSaveAs();
        void slotFileSaveAll();
        /** asks for saving if the file is modified, then closes the actual file and window*/
        void slotFileClose();
        void slotFileClose( Doc* doc );
        void slotFileCloseAll();

        void slotSettingsConfigure();

        /** checks if the currently visible tab is a k3bview
            or not and dis- or enables some actions */
        void slotCurrentDocChanged();

        void slotFileQuit();

        /** toggles the statusbar
         */
        void slotViewStatusBar();

        void slotViewLockPanels( bool checked );

        /** changes the statusbar contents for the standard label permanently, used to indicate current actions.
         * @param text the text that is displayed in the statusbar
         */
        void slotStatusMsg(const QString &text);

        void slotShowMenuBar();

        void slotProjectAddFiles();

        void slotEditToolbars();
        void slotNewToolBarConfig();

        void slotDataImportSession();
        void slotDataClearImportedSession();
        void slotEditBootImages();

        void createClient(K3b::Doc* doc);

        /**
         * Run slotCheckSystem with a timer
         */
        void slotCheckSystemTimed();

    private:
        bool fileSave( Doc* doc = 0 );
        bool fileSaveAs( Doc* doc = 0 );
        void closeProject( Doc* );

        /** save general Options like all bar positions and status as well as the geometry and the recent file list to the configuration
         * file
         */
        void saveOptions();
        /** read general Options again and initialize all variables like the recent file list */
        void readOptions();

        /** initializes the KActions of the application */
        void initActions();

        /** sets up the statusbar for the main window by initialzing a statuslabel.
         */
        void initStatusBar();

        class Private;
        Private* d;
    };
}

#endif // K3B_H
