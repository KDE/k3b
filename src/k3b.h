/***************************************************************************
                          k3b.h  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3B_H
#define K3B_H
 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstrlist.h>
#include <qworkspace.h>

// include files for KDE
#include <kapp.h>
#include <kdockwidget.h>
#include <kaction.h>
#include <kurl.h>

class QVBox;


// forward declaration of the K3b classes
class K3bMainWindow;
class K3bDoc;
class K3bView;
class K3bDirView;
class K3bDeviceManager;
class K3bExternalBinManager;
class K3bOptionDialog;
class K3bJob;
class K3bProjectTabWidget;
class K3bSongManager;
class K3bAudioPlayer;



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
class K3bMainWindow : public KDockMainWindow
{
  Q_OBJECT

 public:
  /** construtor of K3bMainWindow, calls all init functions to create the application.
   * @see initMenuBar initToolBar
   */
  K3bMainWindow();
  ~K3bMainWindow();

  /** opens a file specified by commandline option */
  void openDocumentFile(const KURL& url=KURL());

  K3bDeviceManager*      deviceManager()      { return m_deviceManager; }
  K3bExternalBinManager* externalBinManager() { return m_externalBinManager; }
  K3bSongManager*        songManager()        { return m_songManager; }
  K3bAudioPlayer*        audioPlayer()        { return m_audioPlayer; }
  KConfig*               config()             { return m_config; }

  /**
   * @returns a pointer to the currently visible view or 0 if no project was created
   */
  K3bView* activeView() const;
  /**
   * @returns a pointer to the doc associated with the currently visible view or 0 if no project was created
   */
  K3bDoc* activeDoc() const;

  /** does some initialisation like searching for external programs */
  void init();
	
  /** returns a free temp filename in the given directory
   * @parm dir the directory where to find the tempfile, should end with '/' **/
  QString findTempFile( const QString& ending, const QString& dir = QString::null );

  bool eject();
  void showOptionDialog( int = 0 );
  bool useID3TagForMp3Renaming() const { return m_useID3TagForMp3Renaming; }
  void setUseID3TagForMp3Renaming( bool b ) { m_useID3TagForMp3Renaming = b; }

  /** Creates the main view of the KDockMainWindow instance and initializes the MDI view area including any needed
   *  connections.
   *  must be called after construction
   */
  void initView();

 signals:
  void initializationInfo( const QString& );
  void configChanged( KConfig* c );

  /**
   * if there is some config stuff to save connect to this signal
   */
  void saveConfig( KConfig* c );

 public slots:
  /** No descriptions */
  void slotErrorMessage(const QString&);
  /** No descriptions */
  void slotWarningMessage(const QString&);

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

  /** event filter to catch close events for MDI child windows and is installed in createClient() on every child window.
   * Closing a window calls the eventFilter first which removes the view from the connected documents' view list. If the
   * last view is going to be closed, the eventFilter() tests if the document is modified; if yes, it asks the user to
   * save the document. If the document title contains "Untitled", slotFileSaveAs() gets called to get a save name and path.
   */
  virtual bool eventFilter(QObject* object, QEvent* event);

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
   * checks if doc is modified and asks the user for saving if so.
   * returns false if the user chose cancel.
   */
  bool canCloseDocument( K3bDoc* );

  virtual void showEvent( QShowEvent* e );

 private slots:
  /** clears the document in the actual view to reuse it as the new document */
  void slotFileNew();
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
	
  void slotFileExport();
  void slotFileBurn();
  void slotDirDockHidden();
  void slotSettingsConfigure();
	
  /** checks if the currently visible tab is a k3bview
      or not and dis- or enables some actions */
  void slotCurrentDocChanged( QWidget* w );

  void slotFileQuit();

  /** toggles the toolbar
   */
  void slotViewToolBar();
  /** toggles the statusbar
   */
  void slotViewStatusBar();

  void slotViewAudioPlayer();
  void slotAudioPlayerHidden();
  void slotCheckDockWidgetStatus();

  /** changes the statusbar contents for the standard label permanently, used to indicate current actions.
   * @param text the text that is displayed in the statusbar
   */
  void slotStatusMsg(const QString &text);

  void slotShowDirView();

  void slotBlankCdrw();
  void slotWriteIsoImage();
  // encoding dialog for transcode encoding utility
  void slotDivxEncoding();
  void slotCdCopy();
  void slotK3bSetup();

  void slotNewAudioDoc();
  void slotNewDataDoc();

  void slotProjectAddFiles();

  void slotEditToolbars();

 private:
  void fileSave( K3bDoc* doc = 0 );
  void fileSaveAs( K3bDoc* doc = 0 );

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
  int untitledCount;
  /** a list of all open documents. If the last window of a document gets closed, the installed eventFilter
   * removes this document from the list. The document list is checked for modified documents when the user
   * is about to close the application. */
  QList<K3bDoc> *pDocList;	

  K3bDeviceManager*      m_deviceManager;
  K3bExternalBinManager* m_externalBinManager;
  K3bSongManager*        m_songManager;
  K3bAudioPlayer*        m_audioPlayer;

  // KAction pointers to enable/disable actions
  KActionMenu* actionFileNewMenu;
  KAction* actionFileNewAudio;
  KAction* actionFileNewData;
  KAction* actionFileOpen;
  KRecentFilesAction* actionFileOpenRecent;
  KAction* actionFileSave;
  KAction* actionFileSaveAs;
  KAction* actionFileClose;
  KAction* actionFileQuit;
  KAction* actionFileBurn;
  KAction* actionSettingsConfigure;
  KAction* actionSettingsK3bSetup;
  KAction* actionFileExport;
  KAction* actionToolsBlankCdrw;
  KAction* actionToolsDivxEncoding;
  KAction* actionToolsWriteIsoImage;
  KAction* actionCdCopy;
  KAction* actionProjectAddFiles;
  KToggleAction* actionViewToolBar;
  KToggleAction* actionViewStatusBar;
  KToggleAction* actionViewDirView;
  KToggleAction* actionViewAudioPlayer;

  KDockWidget* mainDock;
  KDockWidget* dirDock;
  KDockWidget* m_audioPlayerDock;
		
  // The K3b-specific widgets
  K3bDirView* m_dirView;
  K3bOptionDialog* m_optionDialog;
	
  bool m_useID3TagForMp3Renaming;
  bool m_initialized;
};

#endif // K3B_H
