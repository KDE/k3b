/*
 *
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
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



#ifndef K3B_H
#define K3B_H


#include <config-k3b.h>

// include files for Qt
#include <qworkspace.h>
//Added by qt3to4:
#include <QShowEvent>
#include <QDockWidget>

// include files for KDE
#include <kapplication.h>
#include <kxmlguiwindow.h>
#include <k3dockwidget.h>
#include <kaction.h>
#include <dockmainwindow3.h>
#include <kurl.h>
#include <kvbox.h>



// forward declaration of the K3b classes
class K3bDoc;
class K3bView;
class K3bDirView;
class K3bExternalBinManager;
class K3bOptionDialog;
class K3bProjectTabWidget;
class KSystemTray;
class K3bStatusBarManager;
class K3bThemedHeader;
class KToggleAction;
class KAction;
class KRecentFilesAction;
class KActionMenu;

namespace K3bDevice {
    class DeviceManager;
    class Device;
}


class K3bMainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    /** construtor of K3bMainWindow, calls all init functions to create the application.
     * @see initMenuBar initToolBar
     */
    K3bMainWindow();
    ~K3bMainWindow();

    /** opens a file specified by commandline option */
    K3bDoc* openDocument( const KUrl& url = KUrl() );

    K3bDevice::DeviceManager*      deviceManager() const;
    K3bExternalBinManager* externalBinManager() const;
    KSharedConfig::Ptr               config() const            { return m_config; }
    // return main window with browser/cd/dvd view, used for DND
    K3bDirView*            mainWindow() const        { return m_dirView; }
    /**
     * @returns a pointer to the currently visible view or 0 if no project was created
     */
    K3bView* activeView() const;
    /**
     * @returns a pointer to the doc associated with the currently visible view or 0 if no project was created
     */
    K3bDoc* activeDoc() const;

    QList<K3bDoc*> projects() const;

    bool eject();
    void showOptionDialog( int = 0 );

    /** Creates the main view of the KDockMainWindow instance and initializes the MDI view area including any needed
     *  connections.
     *  must be called after construction
     */
    void initView();

    KSystemTray* systemTray() const { return m_systemTray; }

public Q_SLOTS:
    K3bDoc* slotNewAudioDoc();
    K3bDoc* slotNewDataDoc();
    K3bDoc* slotNewMixedDoc();
    K3bDoc* slotNewVcdDoc();
    K3bDoc* slotNewMovixDoc();
    K3bDoc* slotNewVideoDvdDoc();
    K3bDoc* slotContinueMultisession();

    void slotClearProject();

    void slotWriteImage();
    void slotWriteImage( const KUrl& url );
    void formatMedium( K3bDevice::Device* );
    void slotFormatMedium();
    void mediaCopy( K3bDevice::Device* );
    void slotMediaCopy();
    void cddaRip( K3bDevice::Device* );
    void slotCddaRip();
    void videoDvdRip( K3bDevice::Device* );
    void slotVideoDvdRip();
    void videoCdRip( K3bDevice::Device* );
    void slotVideoCdRip();
    void slotK3bSetup();

    void showDiskInfo( K3bDevice::Device* );

    void slotErrorMessage(const QString&);
    void slotWarningMessage(const QString&);

    void slotConfigureKeys();
    void slotShowTips();
    void slotCheckSystem();
    void slotManualCheckSystem();

    void addUrls( const KUrl::List& urls );

 Q_SIGNALS:
    void initializationInfo( const QString& );
    void configChanged( KConfig* c );

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
    bool canCloseDocument( K3bDoc* );

    virtual void showEvent( QShowEvent* e );

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
    void slotFileCloseAll();

    void slotSettingsConfigure();

    /** checks if the currently visible tab is a k3bview
	or not and dis- or enables some actions */
    void slotCurrentDocChanged();

    void slotFileQuit();

    /** toggles the statusbar
     */
    void slotViewStatusBar();

    void slotViewDocumentHeader();

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

    void createClient(K3bDoc* doc);

    /**
     * Run slotCheckSystem with a timer
     */
    void slotCheckSystemTimed();

private:
    void fileSave( K3bDoc* doc = 0 );
    void fileSaveAs( K3bDoc* doc = 0 );
    void closeProject( K3bDoc* );

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

    bool isCdDvdImageAndIfSoOpenDialog( const KUrl& url );

    /** the configuration object of the application */
    KSharedConfig::Ptr m_config;

    /** The MDI-Interface is managed by this tabbed view */
    K3bProjectTabWidget* m_documentTab;

    // KAction pointers to enable/disable actions
    KActionMenu* actionFileNewMenu;
    KAction* actionFileNewAudio;
    KAction* actionFileNewData;
    KAction* actionFileNewMixed;
    KAction* actionFileNewVcd;
    KAction* actionFileNewMovix;
    KAction* actionFileNewVideoDvd;
    KAction* actionFileContinueMultisession;
    KAction* actionFileOpen;
    KRecentFilesAction* actionFileOpenRecent;
    KAction* actionFileSave;
    KAction* actionFileSaveAs;
    KAction* actionFileSaveAll;
    KAction* actionFileClose;
    KAction* actionFileCloseAll;
    KAction* actionFileQuit;
    KAction* actionSettingsConfigure;
    KAction* actionSettingsK3bSetup;
    KAction* actionToolsWriteImage;
    KAction* actionToolsCddaRip;
    KAction* actionToolsVideoDvdRip;
    KAction* actionToolsVideoCdRip;
    KAction* actionProjectAddFiles;
    KToggleAction* actionViewStatusBar;
    KToggleAction* actionViewDocumentHeader;

    // project actions
    QList<KAction*> m_dataProjectActions;

    QDockWidget* m_contentsDock;
    QDockWidget* m_dirTreeDock;

    // The K3b-specific widgets
    K3bDirView* m_dirView;
    K3bOptionDialog* m_optionDialog;

    K3bStatusBarManager* m_statusBarManager;

    KSystemTray* m_systemTray;

    bool m_initialized;

    // the funny header
    K3bThemedHeader* m_documentHeader;

    class Private;
    Private* d;
};

#endif // K3B_H
