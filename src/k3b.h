/*
 *
 * $Id$
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef K3B_H
#define K3B_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstrlist.h>
#include <qworkspace.h>
#include <qptrlist.h>

// include files for KDE
#include <kapplication.h>
#include <kparts/dockmainwindow.h>
#include <kdockwidget.h>
#include <kaction.h>
#include <kurl.h>

class QVBox;


// forward declaration of the K3b classes
class K3bMainWindow;
class K3bDoc;
class K3bView;
class K3bDirView;
class K3bExternalBinManager;
class K3bOptionDialog;
class K3bJob;
class K3bProjectTabWidget;
class K3bSongManager;
class K3bBusyWidget;
class KSystemTray;
class K3bStatusBarManager;
class K3bProjectInterface;


namespace K3bDevice {
  class DeviceManager;
  class Device;
}


class K3bMainWindow : public KParts::DockMainWindow
{
  Q_OBJECT

 public:
  /** construtor of K3bMainWindow, calls all init functions to create the application.
   * @see initMenuBar initToolBar
   */
  K3bMainWindow();
  ~K3bMainWindow();

  /** opens a file specified by commandline option */
  K3bDoc* openDocument( const KURL& url = KURL() );

  K3bDevice::DeviceManager*      deviceManager() const;
  K3bExternalBinManager* externalBinManager() const;
  KConfig*               config() const            { return m_config; }
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

  const QPtrList<K3bDoc>& projects() const;

  bool eject();
  void showOptionDialog( int = 0 );

  /** Creates the main view of the KDockMainWindow instance and initializes the MDI view area including any needed
   *  connections.
   *  must be called after construction
   */
  void initView();

  KSystemTray* systemTray() const { return m_systemTray; }

 public slots:
  K3bDoc* slotNewAudioDoc();
  K3bDoc* slotNewDataDoc();
  K3bDoc* slotNewMixedDoc();
  K3bDoc* slotNewVcdDoc();
  K3bDoc* slotNewMovixDoc();
  K3bDoc* slotNewMovixDvdDoc();
  K3bDoc* slotNewDvdDoc();
  K3bDoc* slotNewVideoDvdDoc();

  void slotClearProject();

  void blankCdrw( K3bDevice::Device* );
  void slotBlankCdrw();
  void formatDvd( K3bDevice::Device* );
  void slotFormatDvd();
  void slotWriteCdImage();
  void slotWriteCdImage( const KURL& url );
  void slotWriteDvdIsoImage();
  void slotWriteDvdIsoImage( const KURL& url );
  void cdCopy( K3bDevice::Device* );
  void slotCdCopy();
  void dvdCopy( K3bDevice::Device* );
  void slotDvdCopy();
  void cddaRip( K3bDevice::Device* );
  void slotCddaRip();
  void videoDvdRip( K3bDevice::Device* );
  void slotVideoDvdRip();
  void slotK3bSetup();

  void slotErrorMessage(const QString&);
  void slotWarningMessage(const QString&);

  void slotConfigureKeys();
  void slotShowTips();
  void slotCheckSystem();

  void addUrls( const KURL::List& urls );

 signals:
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
  virtual void saveProperties(KConfig *_cfg);

  /** reads the session config file and restores the application's state including the last opened files and documents by reading the
   * temporary files saved by saveProperties()
   * @see KTMainWindow#readProperties
   */
  virtual void readProperties(KConfig *_cfg);

  /**
   * checks if doc is modified and asks the user for saving if so.
   * returns false if the user chose cancel.
   */
  bool canCloseDocument( K3bDoc* );

  virtual void showEvent( QShowEvent* e );

 private slots:
  /** open a file and load it into the document*/
  void slotFileOpen();
  /** opens a file from the recent files menu */
  void slotFileOpenRecent(const KURL& url);
  /** save a document */
  void slotFileSave();
  /** save a document by a new filename*/
  void slotFileSaveAs();
  void slotFileSaveAll();
  /** asks for saving if the file is modified, then closes the actual file and window*/
  void slotFileClose();
  void slotFileCloseAll();

  void slotDirTreeDockHidden();
  void slotContentsDockHidden();

  void slotSettingsConfigure();

  /** checks if the currently visible tab is a k3bview
      or not and dis- or enables some actions */
  void slotCurrentDocChanged();

  void slotFileQuit();

  /** toggles the statusbar
   */
  void slotViewStatusBar();

  void slotViewDocumentHeader();

  void slotCheckDockWidgetStatus();

  /** changes the statusbar contents for the standard label permanently, used to indicate current actions.
   * @param text the text that is displayed in the statusbar
   */
  void slotStatusMsg(const QString &text);

  void slotShowDirTreeView();
  void slotShowContentsView();
  void slotShowMenuBar();

  void slotProjectAddFiles();

  void slotEditToolbars();
  void slotNewToolBarConfig();

  void slotDataImportSession();
  void slotDataClearImportedSession();
  void slotEditBootImages();

  void showBusyInfo( const QString& str );
  void endBusy();

  void slotThemeChanged();

  void createClient(K3bDoc* doc);

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

  bool isCdDvdImageAndIfSoOpenDialog( const KURL& url );

  /** the configuration object of the application */
  KConfig *m_config;

  /** The MDI-Interface is managed by this tabbed view */
  K3bProjectTabWidget* m_documentTab;

  // KAction pointers to enable/disable actions
  KActionMenu* actionFileNewMenu;
  KAction* actionFileNewAudio;
  KAction* actionFileNewData;
  KAction* actionFileNewMixed;
  KAction* actionFileNewVcd;
  KAction* actionFileNewMovix;
  KAction* actionFileNewMovixDvd;
  KAction* actionFileNewDvd;
  KAction* actionFileNewVideoDvd;
  KAction* actionFileOpen;
  KRecentFilesAction* actionFileOpenRecent;
  KAction* actionFileSave;
  KAction* actionFileSaveAs;
  KAction* actionFileSaveAll;
  KAction* actionFileClose;
  KAction* actionFileCloseAll;
  KAction* actionFileQuit;
  KAction* actionFileBurn;
  KAction* actionSettingsConfigure;
  KAction* actionSettingsK3bSetup;
  KAction* actionToolsBlankCdrw;
  KAction* actionToolsWriteCdImage;
  KAction* actionToolsCddaRip;
  KAction* actionToolsVideoDvdRip;
  KAction* actionCdCopy;
  KAction* actionProjectAddFiles;
  KToggleAction* actionViewStatusBar;
  KToggleAction* actionViewDirTreeView;
  KToggleAction* actionViewContentsView;
  KToggleAction* actionViewDocumentHeader;

  // project actions
  KAction* actionDataImportSession;
  KAction* actionDataClearImportedSession;
  KAction* actionDataEditBootImages;

  QPtrList<KAction> m_dataProjectActions;

  KDockWidget* mainDock;
  KDockWidget* m_contentsDock;
  KDockWidget* m_dirTreeDock;

  // The K3b-specific widgets
  K3bDirView* m_dirView;
  K3bOptionDialog* m_optionDialog;

  K3bStatusBarManager* m_statusBarManager;

  KSystemTray* m_systemTray;

  bool m_initialized;

  // the funny header
  QWidget* m_documentHeader;

  class Private;
  Private* d;
};

#endif // K3B_H
