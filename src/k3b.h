/*
 *
 * $Id$
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
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
class K3bAudioPlayer;
class K3bBusyWidget;
class KSystemTray;
class K3bStatusBarManager;
class K3bProjectInterface;


namespace K3bCdDevice {
  class DeviceManager;
}

/** Access to the "lonely" K3bMainWindow Object */
K3bMainWindow* k3bMain();


/**
  * The base class for K3b application windows. It sets up the main
  * window and reads the config file as well as providing a menubar, toolbar
  * and statusbar. In initView(), your main view is created as the MDI child window manager.
  * Child windows are created in createClient(), which gets a document instance as it's document to
  * display whereby one document can have several views.The MDI child is an instance of K3bView,
  * the document an instance of K3bDoc.
  * K3bMainWindow reimplements the methods that KDockMainWindow provides for main window handling and supports
  * full session management as well as keyboard accelerator configuration by using KAccel.
  * @see KDockMainWindow
  * @see KApplication
  * @see KConfig
  * @see KAccel
  *
  * @author Sebastian Trueg
  */
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

  K3bCdDevice::DeviceManager*      deviceManager() const;
  K3bExternalBinManager* externalBinManager() const;
  K3bAudioPlayer*        audioPlayer() const       { return m_audioPlayer; }
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

  K3bProjectInterface* dcopInterface( K3bDoc* doc );

 public slots:
  K3bDoc* slotNewAudioDoc();
  K3bDoc* slotNewDataDoc();
  K3bDoc* slotNewMixedDoc();
  K3bDoc* slotNewVcdDoc();
  K3bDoc* slotNewMovixDoc();
  K3bDoc* slotNewMovixDvdDoc();
  K3bDoc* slotNewDvdDoc();

  void slotBlankCdrw();
  void slotFormatDvd();
  void slotWriteCdIsoImage();
  void slotWriteDvdIsoImage();
  void slotWriteIsoImage( const KURL& url );
  void slotWriteBinImage();
  void slotWriteBinImage( const KURL& url );
  // encoding dialog for transcode encoding utility
  void slotDivxEncoding();
  void slotCdCopy();
  void slotCdClone();
  void slotDvdCopy();
  void slotK3bSetup();
  /** No descriptions */
  void slotErrorMessage(const QString&);
  /** No descriptions */
  void slotWarningMessage(const QString&);

  void slotViewAudioPlayer();

  void slotConfigureKeys();
  void slotShowTips();

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

  /** creates a new child window. The document that will be connected to it
   * has to be created before and the instances filled, with e.g. openDocument().
   * Then call createClient() to get a new MDI child window.
   * @see K3bDoc#addView
   * @see K3bDoc#openDocument
   * @param doc pointer to the document instance that the view will
   * be connected to.
   */
  void createClient(K3bDoc* doc);

  /**
   * Initializes a newly created doc.
   * loads the default settings and adds
   * it to the list of documents.
   */
  void initializeNewDoc( K3bDoc* doc );

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
  /** asks for saving if the file is modified, then closes the actual file and window*/
  void slotFileClose();

  void slotDirTreeDockHidden();
  void slotContentsDockHidden();
  void slotProjectDockHidden();

  void slotSettingsConfigure();

  /** checks if the currently visible tab is a k3bview
      or not and dis- or enables some actions */
  void slotCurrentDocChanged();

  void slotFileQuit();

  /** toggles the statusbar
   */
  void slotViewStatusBar();

  void slotViewDocumentHeader();

  void slotAudioPlayerHidden();
  void slotCheckDockWidgetStatus();

  /** changes the statusbar contents for the standard label permanently, used to indicate current actions.
   * @param text the text that is displayed in the statusbar
   */
  void slotStatusMsg(const QString &text);

  void slotShowDirTreeView();
  void slotShowContentsView();
  void slotShowProjectView();

  void slotProjectAddFiles();

  void slotEditToolbars();
  void slotNewToolBarConfig();

  void slotDataImportSession();
  void slotDataClearImportedSession();
  void slotEditBootImages();

  void setProjectsHidable( bool );

  void showBusyInfo( const QString& str );
  void endBusy();

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

  /** the configuration object of the application */
  KConfig *m_config;

  /** The MDI-Interface is managed by this tabbed view */
  K3bProjectTabWidget* m_documentTab;

  /** a counter that gets increased each time the user creates a new document with "File"->"New" */
  int m_audioUntitledCount;
  int m_dataUntitledCount;
  int m_mixedUntitledCount;
  int m_vcdUntitledCount;
  int m_movixUntitledCount;
  int m_movixDvdUntitledCount;
  int m_dvdUntitledCount;

  K3bAudioPlayer*        m_audioPlayer;

  // KAction pointers to enable/disable actions
  KActionMenu* actionFileNewMenu;
  KAction* actionFileNewAudio;
  KAction* actionFileNewData;
  KAction* actionFileNewMixed;
  KAction* actionFileNewVcd;
  KAction* actionFileNewMovix;
  KAction* actionFileNewMovixDvd;
  KAction* actionFileNewDvd;
  KAction* actionFileOpen;
  KRecentFilesAction* actionFileOpenRecent;
  KAction* actionFileSave;
  KAction* actionFileSaveAs;
  KAction* actionFileClose;
  KAction* actionFileQuit;
  KAction* actionFileBurn;
  KAction* actionSettingsConfigure;
  KAction* actionSettingsK3bSetup;
  KAction* actionToolsBlankCdrw;
  KAction* actionToolsDivxEncoding;
  KAction* actionToolsWriteIsoImage;
  KAction* actionToolsWriteBinImage;
  KAction* actionCdCopy;
  KAction* actionProjectAddFiles;
  KToggleAction* actionViewStatusBar;
  KToggleAction* actionViewDirTreeView;
  KToggleAction* actionViewContentsView;
  KToggleAction* actionViewProjectView;
  KToggleAction* actionViewAudioPlayer;
  KToggleAction* actionViewDocumentHeader;

  // project actions
  KAction* actionDataImportSession;
  KAction* actionDataClearImportedSession;
  KAction* actionDataEditBootImages;

  QPtrList<KAction> m_dataProjectActions;

  KDockWidget* mainDock;
  KDockWidget* m_contentsDock;
  KDockWidget* m_dirTreeDock;
  KDockWidget* m_audioPlayerDock;

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
