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

#include <config.h>


// include files for QT
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qtooltip.h>
#include <qtoolbutton.h>
#include <qstring.h>
#include <qsplitter.h>
#include <qevent.h>
#include <qvaluelist.h>
#include <qfont.h>
#include <qpalette.h>
#include <qmap.h>
#include <qwidgetstack.h>

#include <kkeydialog.h>
// include files for KDE
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <klineeditdlg.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <kurl.h>
#include <kurllabel.h>
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <kglobalsettings.h>
#include <kdialog.h>
#include <kedittoolbar.h>
#include <ksystemtray.h>
#include <kaboutdata.h>
#include <ktip.h>
#include <kxmlguifactory.h>
#include <kstdguiitem.h>

#include <stdlib.h>

// application specific includes
#include "k3b.h"
#include "k3bcore.h"
#include <k3bglobals.h>
#include "k3bview.h"
#include "k3bdirview.h"
#include "audiocd/k3baudiodoc.h"
#include "audiocd/k3baudioview.h"
#include "device/k3bdevicemanager.h"
#include "audiocd/k3baudiotrackdialog.h"
#include "option/k3boptiondialog.h"
#include "k3bprojectburndialog.h"
#include "datacd/k3bdatadoc.h"
#include "datadvd/k3bdvddoc.h"
#include "videodvd/k3bvideodvddoc.h"
#include "datacd/k3bdataview.h"
#include "mixedcd/k3bmixeddoc.h"
#include "videocd/k3bvcddoc.h"
#include "movixcd/k3bmovixdoc.h"
#include "movixdvd/k3bmovixdvddoc.h"
#include "k3bblankingdialog.h"
#include "images/k3bcdimagewritingdialog.h"
#include "images/k3bisoimagewritingdialog.h"
#include <k3bexternalbinmanager.h>
#include "k3bprojecttabwidget.h"
#include "k3baudioplayer.h"
#include "cdcopy/k3bcdcopydialog.h"
#include "videoEncoding/k3bdivxview.h"
#include "k3btempdirselectionwidget.h"
#include "tools/k3bbusywidget.h"
#include "k3bstatusbarmanager.h"
#include "k3bfiletreecombobox.h"
#include "k3bfiletreeview.h"
#include "k3bstdguiitems.h"
#include "datadvd/k3bdvdformattingdialog.h"
#include "dvdcopy/k3bdvdcopydialog.h"
//#include "dvdcopy/k3bvideodvdcopydialog.h"
#include "k3bprojectinterface.h"
#include <k3bprojectmanager.h>
#include "k3bwelcomewidget.h"
#include "plugin/k3bpluginmanager.h"
#include "plugin/k3bpluginfactory.h"
#include <k3bsystemproblemdialog.h>
#include <k3baudiodecoder.h>
#include <k3bthememanager.h>



static K3bMainWindow* s_k3bMainWindow = 0;

K3bMainWindow* k3bMain()
{
  if( !s_k3bMainWindow ) {
    kdDebug() << "No K3bMainWindow found!" << endl;
    exit(1);
  }
  return s_k3bMainWindow;
}



class K3bMainWindow::Private
{
public:
  QMap<K3bDoc*, K3bProjectInterface*> projectInterfaceMap;

  K3bProjectManager* projectManager;
  K3bDoc* lastDoc;

  QWidgetStack* documentStack;
  K3bWelcomeWidget* welcomeWidget;
  QWidget* documentHull;

  QLabel* leftDocPicLabel;
  QLabel* centerDocLabel;
  QLabel* rightDocPicLabel;
};


K3bMainWindow::K3bMainWindow()
  : DockMainWindow(0,"K3b")
{
  d = new Private;
  d->projectManager = new K3bProjectManager( this );
  d->lastDoc = 0;

  s_k3bMainWindow = this;

  setPlainCaption( i18n("K3b - The CD Kreator") );

  m_config = kapp->config();
  m_audioUntitledCount = 0;
  m_dataUntitledCount = 0;
  m_mixedUntitledCount = 0;
  m_vcdUntitledCount = 0;
  m_movixUntitledCount = 0;
  m_movixDvdUntitledCount = 0;
  m_dvdUntitledCount = 0;
  m_videoDvdUntitledCount = 0;

  //setup splitter behavior
  manager()->setSplitterHighResolution(true);
  manager()->setSplitterOpaqueResize(true);
  manager()->setSplitterKeepSize(true);

  resize(780,520);  // default optimized for 800x600

  ///////////////////////////////////////////////////////////////////
  // call inits to invoke all other construction parts
  initView();
  initStatusBar();
  initActions();
  createGUI(0L);

  // we need the actions for the welcomewidget
  d->welcomeWidget->loadConfig( config() );

  // fill the tabs action menu
  m_documentTab->insertAction( actionFileSave );
  m_documentTab->insertAction( actionFileSaveAs );
  m_documentTab->insertAction( actionFileClose );

  readOptions();

  // /////////////////////////////////////////////////////////////////
  // disable actions at startup
  slotStateChanged( "state_project_active", KXMLGUIClient::StateReverse );

  // connect to the busy signals
  connect( k3bcore, SIGNAL(busyInfoRequested(const QString&)), this, SLOT(showBusyInfo(const QString&)) );
  connect( k3bcore, SIGNAL(busyFinishRequested()), this, SLOT(endBusy()) );

  connect( k3bthememanager, SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );
}

K3bMainWindow::~K3bMainWindow()
{
  delete mainDock;
  delete m_audioPlayerDock;
  delete m_contentsDock;

  delete d;
}


void K3bMainWindow::showEvent( QShowEvent* e )
{
  slotCheckDockWidgetStatus();
  KDockMainWindow::showEvent( e );
}


void K3bMainWindow::initActions()
{
  actionFileOpen = KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
  actionFileOpenRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const KURL&)), actionCollection());
  actionFileSave = KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
  actionFileSaveAs = KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  actionFileClose = KStdAction::close(this, SLOT(slotFileClose()), actionCollection());
  actionFileQuit = KStdAction::quit(this, SLOT(slotFileQuit()), actionCollection());
  actionViewStatusBar = KStdAction::showStatusbar(this, SLOT(slotViewStatusBar()), actionCollection());
  actionSettingsConfigure = KStdAction::preferences(this, SLOT(slotSettingsConfigure()), actionCollection() );

  // the tip action
  (void)KStdAction::tipOfDay(this, SLOT(slotShowTips()), actionCollection() );
  (void)KStdAction::keyBindings( this, SLOT( slotConfigureKeys() ), actionCollection() );

  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
  setStandardToolBarMenuEnabled(true);

  actionFileNewMenu = new KActionMenu( i18n("&New Project"), "filenew", actionCollection(), "file_new" );
  actionFileNewAudio = new KAction(i18n("New &Audio CD Project"), "sound", 0, this, SLOT(slotNewAudioDoc()),
			     actionCollection(), "file_new_audio");
  actionFileNewData = new KAction(i18n("New Data &CD Project"),"tar", 0, this, SLOT(slotNewDataDoc()),
			    actionCollection(), "file_new_data");
  actionFileNewMixed = new KAction(i18n("New &Mixed Mode CD Project"),"tar", 0, this, SLOT(slotNewMixedDoc()),
				   actionCollection(), "file_new_mixed");
  actionFileNewVcd = new KAction(i18n("New &Video CD Project"),"video", 0, this, SLOT(slotNewVcdDoc()),
				   actionCollection(), "file_new_vcd");
  actionFileNewMovix = new KAction(i18n("New &eMovix CD Project"),"video", 0, this, SLOT(slotNewMovixDoc()),
				   actionCollection(), "file_new_movix");
  actionFileNewMovixDvd = new KAction(i18n("New &eMovix DVD Project"),"video", 0, this, SLOT(slotNewMovixDvdDoc()),
				      actionCollection(), "file_new_movix_dvd");
  actionFileNewDvd = new KAction(i18n("New Data &DVD Project"), "dvd_unmount", 0, this, SLOT(slotNewDvdDoc()),
				 actionCollection(), "file_new_dvd");
  actionFileNewVideoDvd = new KAction(i18n("New V&ideo DVD Project"), "video", 0, this, SLOT(slotNewVideoDvdDoc()),
				      actionCollection(), "file_new_video_dvd");


  actionFileNewMenu->insert( actionFileNewAudio );
  actionFileNewMenu->insert( actionFileNewData );
  actionFileNewMenu->insert( actionFileNewMixed );
  actionFileNewMenu->insert( actionFileNewVcd );
  actionFileNewMenu->insert( actionFileNewMovix );
  actionFileNewMenu->insert( actionFileNewDvd );
  actionFileNewMenu->insert( actionFileNewVideoDvd );
  actionFileNewMenu->insert( actionFileNewMovixDvd );
  actionFileNewMenu->setDelayed( false );

  actionProjectAddFiles = new KAction( i18n("&Add Files..."), "filenew", 0, this, SLOT(slotProjectAddFiles()),
				       actionCollection(), "project_add_files");

  (void)new KAction( i18n("&Clear Project"), "clear_left", 0, this, SLOT(slotClearProject()),
		     actionCollection(), "project_clear_project" );

  actionViewDirTreeView = new KToggleAction(i18n("Show Directories"), 0, this, SLOT(slotShowDirTreeView()),
					    actionCollection(), "view_dir_tree");

  actionViewContentsView = new KToggleAction(i18n("Show Contents"), 0, this, SLOT(slotShowContentsView()),
					     actionCollection(), "view_contents");

  actionViewProjectView = new KToggleAction(i18n("Show Project View"), 0, this, SLOT(slotShowProjectView()),
					    actionCollection(), "view_show_project_view");

  actionViewAudioPlayer = new KToggleAction(i18n("Show Audio Player"), 0, this, SLOT(slotViewAudioPlayer()),
					    actionCollection(), "view_audio_player");

  actionViewDocumentHeader = new KToggleAction(i18n("Show Document Header"), 0, this, SLOT(slotViewDocumentHeader()),
					       actionCollection(), "view_document_header");

  actionToolsBlankCdrw = new KAction( i18n("&Erase CD-RW..."), "cdrwblank", 0, this, SLOT(slotBlankCdrw()),
				      actionCollection(), "tools_blank_cdrw" );
  /*KAction* actionToolsFormatDVD = */(void)new KAction( i18n("&Format DVD-RW/DVD+RW..."), "cdrwblank", 0, this, SLOT(slotFormatDvd()),
							 actionCollection(), "tools_format_dvd" );
  actionToolsDivxEncoding = new KAction(i18n("&Encode Video..."),"gear", 0, this, SLOT( slotDivxEncoding() ),
			    actionCollection(), "tools_encode_video");
  actionToolsWriteCdImage = new KAction(i18n("&Burn CD Image..."), "gear", 0, this, SLOT(slotWriteCdImage()),
					 actionCollection(), "tools_write_cd_image" );
  (void)new KAction(i18n("&Burn DVD ISO Image..."), "gear", 0, this, SLOT(slotWriteDvdIsoImage()),
		    actionCollection(), "tools_write_dvd_iso" );

  actionCdCopy = new KAction(i18n("&Copy CD..."), "cdcopy", 0, this, SLOT(slotCdCopy()),
			     actionCollection(), "tools_copy_cd" );

  KAction* actionToolsDvdCopy = new KAction(i18n("Copy &DVD..."), "cdcopy", 0, this, SLOT(slotDvdCopy()),
					    actionCollection(), "tools_copy_dvd" );

//   KAction* actionToolsVideoDvdCopy = new KAction(i18n("Copy &VideoDVD..."), "cdcopy", 0, this, SLOT(slotVideoDvdCopy()),
// 						 actionCollection(), "tools_copy_video_dvd" );

  (void)new KAction( i18n("System Check"), 0, 0, this, SLOT(slotCheckSystem()),
		     actionCollection(), "help_check_system" );

#ifdef HAVE_K3BSETUP
  actionSettingsK3bSetup = new KAction(i18n("K3b &Setup"), "configure", 0, this, SLOT(slotK3bSetup()),
				       actionCollection(), "settings_k3bsetup" );
#endif

  // Project actions (TODO: these should go into K3bDoc and it's subclasses)
  // ==============================================================================================================

  // Data Project
//   actionDataImportSession = new KAction(i18n("&Import Session"), "gear", 0, this, SLOT(slotDataImportSession()),
// 					actionCollection(), "project_data_import_session" );
//   actionDataClearImportedSession = new KAction(i18n("&Clear Imported Session"), "gear", 0, this,
// 					       SLOT(slotDataClearImportedSession()), actionCollection(),
// 					       "project_data_clear_imported_session" );
//   actionDataEditBootImages = new KAction(i18n("&Edit Boot Images"), "cdtrack", 0, this,
//  					 SLOT(slotEditBootImages()), actionCollection(),
//  					 "project_data_edit_boot_images" );

//   m_dataProjectActions.append( actionDataImportSession );
//   m_dataProjectActions.append( actionDataClearImportedSession );
//   m_dataProjectActions.append( actionDataEditBootImages );
  // ==============================================================================================================

  // --- filetreecombobox-toolbar -------------------------------------------------------------------
  K3bFileTreeComboBox* m_fileTreeComboBox = new K3bFileTreeComboBox( 0 );
  connect( m_fileTreeComboBox, SIGNAL(urlExecuted(const KURL&)), m_dirView, SLOT(showUrl(const KURL& )) );
  connect( m_fileTreeComboBox, SIGNAL(deviceExecuted(K3bCdDevice::CdDevice*)), m_dirView,
	   SLOT(showDevice(K3bCdDevice::CdDevice* )) );

  KWidgetAction* fileTreeComboAction = new KWidgetAction( m_fileTreeComboBox,
							  i18n("&Quick Dir Selector"),
							  0, 0, 0,
							  actionCollection(), "quick_dir_selector" );
  fileTreeComboAction->setAutoSized(true);
  (void)new KAction( i18n("Go"), "key_enter", 0, m_fileTreeComboBox, SLOT(slotGoUrl()), actionCollection(), "go_url" );
  // ---------------------------------------------------------------------------------------------



  actionFileNewMenu->setToolTip(i18n("Creates a new project"));
  actionFileNewData->setToolTip( i18n("Creates a new data CD project") );
  actionFileNewAudio->setToolTip( i18n("Creates a new audio CD project") );
  actionFileNewMovixDvd->setToolTip( i18n("Creates a new eMovix DVD project") );
  actionFileNewDvd->setToolTip( i18n("Creates a new data DVD project") );
  actionFileNewMovix->setToolTip( i18n("Creates a new eMovix CD project") );
  actionFileNewVcd->setToolTip( i18n("Creates a new Video CD project") );
  actionToolsBlankCdrw->setToolTip( i18n("Opens CD-blanking dialog") );
  actionCdCopy->setToolTip( i18n("Open the CD Copy dialog") );
  actionToolsDvdCopy->setToolTip( i18n("Open the DVD Copy dialog") );
  //  actionToolsVideoDvdCopy->setToolTip( i18n("Open the VideoDVD Copy dialog") );
  actionFileOpen->setToolTip(i18n("Opens an existing project"));
  actionFileOpenRecent->setToolTip(i18n("Opens a recently used file"));
  actionFileSave->setToolTip(i18n("Saves the actual project"));
  actionFileSaveAs->setToolTip(i18n("Saves the actual project as..."));
  actionFileClose->setToolTip(i18n("Closes the actual project"));
  actionFileQuit->setToolTip(i18n("Quits the application"));
}



const QPtrList<K3bDoc>& K3bMainWindow::projects() const
{
  return d->projectManager->projects();
}


void K3bMainWindow::slotConfigureKeys()
{
  KKeyDialog::configure( actionCollection(), this );
}

void K3bMainWindow::initStatusBar()
{
  m_statusBarManager = new K3bStatusBarManager( this );
}


void K3bMainWindow::showBusyInfo( const QString& str )
{
  m_statusBarManager->showBusyInfo(str);
}


void K3bMainWindow::endBusy()
{
  m_statusBarManager->endBusy();
}


void K3bMainWindow::initView()
{
  // setup main docking things
  mainDock = createDockWidget( "project_view", SmallIcon("idea"), 0,
			       kapp->makeStdCaption( i18n("Project View") ), i18n("Project View") );
  setView( mainDock );
  setMainDockWidget( mainDock );
  connect( mainDock, SIGNAL(iMBeingClosed()), this, SLOT(slotProjectDockHidden()) );
  connect( mainDock, SIGNAL(hasUndocked()), this, SLOT(slotProjectDockHidden()) );


  // --- Document Dock ----------------------------------------------------------------------------
  d->documentStack = new QWidgetStack( mainDock );
  mainDock->setWidget( d->documentStack );

  d->documentHull = new QWidget( d->documentStack );
  d->documentStack->addWidget( d->documentHull );
  QGridLayout* documentHullLayout = new QGridLayout( d->documentHull );
  documentHullLayout->setMargin( 2 );
  documentHullLayout->setSpacing( 0 );

  m_documentHeader = K3bStdGuiItems::purpleFrame( d->documentHull );
  QGridLayout* documentHeaderLayout = new QGridLayout( m_documentHeader );
  documentHeaderLayout->setMargin( 2 );
  documentHeaderLayout->setSpacing( 0 );

  d->leftDocPicLabel = new QLabel( m_documentHeader );
  d->centerDocLabel = new QLabel( m_documentHeader );
  d->rightDocPicLabel = new QLabel( m_documentHeader );

  d->centerDocLabel->setText( i18n("Current Projects") );
  d->centerDocLabel->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  QFont f(d->centerDocLabel->font());
  f.setBold(true);
  f.setPointSize( 12 );
  d->centerDocLabel->setFont(f);

  documentHeaderLayout->addWidget( d->leftDocPicLabel, 0, 0 );
  documentHeaderLayout->addWidget( d->centerDocLabel, 0, 1 );
  documentHeaderLayout->addWidget( d->rightDocPicLabel, 0, 2 );
  documentHeaderLayout->setColStretch( 1, 1 );

  // add the document tab to the styled document box
  m_documentTab = new K3bProjectTabWidget( d->documentHull );
  //  m_documentTab->setPalette( oldPal );

  documentHullLayout->addWidget( m_documentHeader, 0, 0 );
  documentHullLayout->addWidget( m_documentTab, 1, 0 );

  connect( m_documentTab, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotCurrentDocChanged()) );


  d->welcomeWidget = new K3bWelcomeWidget( this, d->documentStack );
//   d->welcomeWidget->m_topPixLabel->setPixmap( QPixmap(locate( "data", "k3b/pics/k3bprojectview_left.png" )) );
//   d->welcomeWidget->m_bottomPixLabel->setPixmap( QPixmap(locate( "data", "k3b/pics/k3bprojectview_right.png" )) );

//   connect( d->welcomeWidget->m_buttonDataCD, SIGNAL(leftClickedURL()),
// 	   this, SLOT(slotNewDataDoc()) );
//   connect( d->welcomeWidget->m_buttonDataDVD, SIGNAL(leftClickedURL()),
// 	   this, SLOT(slotNewDvdDoc()) );
//   connect( d->welcomeWidget->m_buttonAudioCD, SIGNAL(leftClickedURL()),
// 	   this, SLOT(slotNewAudioDoc()) );
//   connect( d->welcomeWidget->m_buttonCopyCD, SIGNAL(leftClickedURL()),
// 	   this, SLOT(slotCdCopy()) );

  d->documentStack->addWidget( d->welcomeWidget );
  d->documentStack->raiseWidget( d->welcomeWidget );
  // ---------------------------------------------------------------------------------------------


  // --- Directory Dock --------------------------------------------------------------------------
  m_dirTreeDock = createDockWidget( "directory_tree", SmallIcon("folder"), 0,
				    kapp->makeStdCaption( i18n("Directory Tree") ), i18n("Directory Tree") );
  K3bFileTreeView* m_fileTreeView = new K3bFileTreeView( m_dirTreeDock );
  m_dirTreeDock->setWidget( m_fileTreeView );
  m_dirTreeDock->manualDock( mainDock, KDockWidget::DockTop, 4000 );
  connect( m_dirTreeDock, SIGNAL(iMBeingClosed()), this, SLOT(slotDirTreeDockHidden()) );
  connect( m_dirTreeDock, SIGNAL(hasUndocked()), this, SLOT(slotDirTreeDockHidden()) );

  m_contentsDock = createDockWidget( "contents_view", SmallIcon("idea"), 0,
			      kapp->makeStdCaption( i18n("Contents View") ), i18n("Contents View") );
  m_dirView = new K3bDirView( m_fileTreeView, m_contentsDock );
  m_contentsDock->setWidget( m_dirView );
  //  m_contentsDock->setEnableDocking( KDockWidget::DockFullDocking/*DockCorner*/ );
  m_contentsDock->manualDock( m_dirTreeDock, KDockWidget::DockRight, 2000 );

  connect( m_contentsDock, SIGNAL(iMBeingClosed()), this, SLOT(slotContentsDockHidden()) );
  connect( m_contentsDock, SIGNAL(hasUndocked()), this, SLOT(slotContentsDockHidden()) );
  // ---------------------------------------------------------------------------------------------


  // --- Audioplayer Dock ------------------------------------------------------------------------
  m_audioPlayerDock = createDockWidget( "audio_player", SmallIcon("1rightarrow"), 0,
					kapp->makeStdCaption( i18n("Audioplayer") ), i18n("Audioplayer") );
  m_audioPlayer = new K3bAudioPlayer( this, "k3b_audio_player" );
  m_audioPlayerDock->setWidget( m_audioPlayer );
  //  m_audioPlayerDock->setEnableDocking( KDockWidget::DockFullDocking/*DockCorner*/ );
  //  m_audioPlayerDock->manualDock( mainDock, KDockWidget::DockRight, 8000 );

  connect( m_audioPlayerDock, SIGNAL(iMBeingClosed()), this, SLOT(slotAudioPlayerHidden()) );
  connect( m_audioPlayerDock, SIGNAL(hasUndocked()), this, SLOT(slotAudioPlayerHidden()) );
  // ---------------------------------------------------------------------------------------------
}


void K3bMainWindow::createClient(K3bDoc* doc)
{
  K3bView* w = doc->createView( m_documentTab );
  m_documentTab->insertTab( w, w->caption(), 0 );
  m_documentTab->showPage( w );

  slotCurrentDocChanged();

  setProjectsHidable( false );
}


K3bView* K3bMainWindow::activeView() const
{
  QWidget* w = m_documentTab->currentPage();
  if( K3bView* view = dynamic_cast<K3bView*>(w) )
    return view;
  else
    return 0;
}


K3bDoc* K3bMainWindow::activeDoc() const
{
  if( activeView() )
    return activeView()->getDocument();
  else
    return 0;
}


K3bDoc* K3bMainWindow::openDocument(const KURL& url)
{
  slotStatusMsg(i18n("Opening file..."));

  // check, if document already open. If yes, set the focus to the first view
  K3bDoc* doc = d->projectManager->findByUrl( url );
  if( doc ) {
    doc->view()->setFocus();
    return doc;
  }

  doc = K3bDoc::openDocument( url );

  if( doc == 0 ) {
    KMessageBox::error (this,i18n("Could not open document!"), i18n("Error!"));
    return 0;
  }

  actionFileOpenRecent->addURL(url);

  // create the window
  createClient(doc);

  return doc;
}


void K3bMainWindow::saveOptions()
{
  m_config->setGroup( "General Options" );

  m_config->writeEntry( "Show Document Header", actionViewDocumentHeader->isChecked() );
  actionFileOpenRecent->saveEntries( m_config, "Recent Files" );

  // save dock positions!
  manager()->writeConfig( m_config, "Docking Config" );

  m_dirView->saveConfig( config() );

  saveMainWindowSettings( m_config, "main_window_settings" );

  k3bcore->saveConfig();

  d->welcomeWidget->saveConfig( config() );
}


void K3bMainWindow::readOptions()
{
  m_config->setGroup("General Options");

  bool bViewDocumentHeader = m_config->readBoolEntry("Show Document Header", true);
  actionViewDocumentHeader->setChecked(bViewDocumentHeader);

  // initialize the recent file list
  actionFileOpenRecent->loadEntries(m_config,"Recent Files");

  // do not read dock-positions from a config that has been saved by an old version
  K3bVersion configVersion( m_config->readEntry( "config version", "0.1" ) );
  if( configVersion >= K3bVersion("0.9") )
    manager()->readConfig( m_config, "Docking Config" );
  else
    kdDebug() << "(K3bMainWindow) ignoring docking config from K3b version " << configVersion << endl;

  applyMainWindowSettings( m_config, "main_window_settings" );

  slotViewDocumentHeader();
  slotCheckDockWidgetStatus();

  slotThemeChanged();
}


void K3bMainWindow::saveProperties( KConfig* )
{

}


void K3bMainWindow::readProperties( KConfig* )
{
}


bool K3bMainWindow::queryClose()
{
  // ---------------------------------
  // we need to manually close all the views to ensure that
  // each of them receives a close-event and
  // the user is asked for every modified doc to save the changes
  // ---------------------------------

  while( K3bView* view = activeView() ) {
    if( !canCloseDocument(view->doc()) )
      return false;
    closeProject(view->doc());
  }

  return true;
}


bool K3bMainWindow::canCloseDocument( K3bDoc* doc )
{
  if( !doc->isModified() ) {
    return true;
  }

  switch ( KMessageBox::warningYesNoCancel(this, i18n("%1 has unsaved data.").arg( doc->URL().fileName() ),
					   i18n("Closing Project"), i18n("&Save"), i18n("&Discard") ) )
    {
    case KMessageBox::Yes:
      fileSave( doc );
    case KMessageBox::No:
      return true;

    default:
      return false;
    }
}

bool K3bMainWindow::queryExit()
{
  saveOptions();
  return true;
}



/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////


void K3bMainWindow::slotFileOpen()
{
  slotStatusMsg(i18n("Opening file..."));

  KURL url=KFileDialog::getOpenURL(QString::null,
				   i18n("*.k3b|K3b Projects"), this, i18n("Open File"));
  if(!url.isEmpty()) {
    openDocument(url);
    actionFileOpenRecent->addURL( url );
  }
}

void K3bMainWindow::slotFileOpenRecent(const KURL& url)
{
  slotStatusMsg(i18n("Opening file..."));

  openDocument(url);
}


void K3bMainWindow::slotFileSave()
{
  if( K3bDoc* doc = activeDoc() ) {
    fileSave( doc );
  }
}

void K3bMainWindow::fileSave( K3bDoc* doc )
{
  slotStatusMsg(i18n("Saving file..."));

  if( doc == 0 ) {
    doc = activeDoc();
  }
  if( doc != 0 ) {
    if( !doc->saved() )
      fileSaveAs( doc );
    else if( !doc->saveDocument(doc->URL()) )
      KMessageBox::error (this,i18n("Could not save the current document!"), i18n("I/O Error"));
  }
}


void K3bMainWindow::slotFileSaveAs()
{
  if( K3bDoc* doc = activeDoc() ) {
    fileSaveAs( doc );
  }
}


void K3bMainWindow::fileSaveAs( K3bDoc* doc )
{
  slotStatusMsg(i18n("Saving file with a new filename..."));

  if( doc == 0 ) {
    doc = activeDoc();
  }

  if( doc != 0 ) {

    QString url = KFileDialog::getSaveFileName(QDir::currentDirPath(),
					       i18n("*.k3b|K3b Projects"), this, i18n("Save As"));


    if( !url.isEmpty() ) {

      // default to ending ".k3b"
      if( url.mid( url.findRev('.')+1 ) != "k3b" ) {
	if( url[ url.length()-1 ] != '.' )
	  url += ".";
	url += "k3b";
      }

      if( !QFile::exists(url) ||
	  ( QFile::exists(url) &&
	    KMessageBox::questionYesNo( this, i18n("Do you want to overwrite %1").arg(url), i18n("File Exists") )
	    == KMessageBox::Yes ) ) {

	if( !doc->saveDocument(url) ) {
	  KMessageBox::error (this,i18n("Could not save the current document!"), i18n("I/O Error"));
	  return;
	}

	K3bView* view = doc->view();
	m_documentTab->changeTab( view, view->caption() );

	actionFileOpenRecent->addURL(url);
      }
    }
  }
}


void K3bMainWindow::slotFileClose()
{
  slotStatusMsg(i18n("Closing file..."));
  if( K3bView* pView = activeView() ) {
    if( pView ) {
      K3bDoc* pDoc = pView->doc();

      if( canCloseDocument(pDoc) ) {
	closeProject(pDoc);
      }
    }
  }

  slotCurrentDocChanged();
}


void K3bMainWindow::closeProject( K3bDoc* doc )
{
  // unplug the actions
  if( factory() ) {
    if( d->lastDoc == doc ) {
      factory()->removeClient( d->lastDoc->view() );
      d->lastDoc = 0;
    }
  }

  // remove the view from the project tab
  m_documentTab->removePage( doc->view() );
  // remove the DCOP interface
  QMap<K3bDoc*, K3bProjectInterface*>::iterator it = d->projectInterfaceMap.find( doc );
  if( it != d->projectInterfaceMap.end() ) {
    // delete the interface
    delete it.data();
    d->projectInterfaceMap.remove( it );
  }

  // delete view and doc
  delete doc->view();
  delete doc;
}


void K3bMainWindow::slotFileQuit()
{
  close();
}


void K3bMainWindow::slotViewStatusBar()
{
  //turn Statusbar on or off
  if(actionViewStatusBar->isChecked()) {
    statusBar()->show();
  }
  else {
    statusBar()->hide();
  }
}


void K3bMainWindow::slotStatusMsg(const QString &text)
{
  ///////////////////////////////////////////////////////////////////
  // change status message permanently
//   statusBar()->clear();
//   statusBar()->changeItem(text,1);

  statusBar()->message( text, 2000 );
}


void K3bMainWindow::slotSettingsConfigure()
{
  K3bOptionDialog d( this, "SettingsDialog", true );

  d.exec();

  // emit a changed signal every time since we do not know if the user selected
  // "apply" and "cancel" or "ok"
  emit configChanged( m_config );
}


void K3bMainWindow::showOptionDialog( int index )
{
  K3bOptionDialog d( this, "SettingsDialog", true );

  d.showPage( index );

  d.exec();

  // emit a changed signal every time since we do not know if the user selected
  // "apply" and "cancel" or "ok"
  emit configChanged( m_config );
}


K3bDoc* K3bMainWindow::slotNewAudioDoc()
{
  if( k3bpluginmanager->factories( "AudioDecoder" ).isEmpty() )
    KMessageBox::error( this, i18n("No audio decoder plugins found. You won't be able to add any files "
				   "to the audio project!") );

  slotStatusMsg(i18n("Creating new Audio CD Project."));

  K3bAudioDoc* doc = new K3bAudioDoc( this );
  initializeNewDoc( doc );

  m_audioUntitledCount++;
  QString fileName = i18n("AudioCD%1").arg(m_audioUntitledCount);
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);

  return doc;
}

K3bDoc* K3bMainWindow::slotNewDataDoc()
{
  slotStatusMsg(i18n("Creating new Data CD Project."));

  K3bDataDoc* doc = new K3bDataDoc( this );
  initializeNewDoc( doc );

  m_dataUntitledCount++;
  QString fileName = i18n("DataCD%1").arg(m_dataUntitledCount);
  if( doc->isoOptions().volumeID().isEmpty() )
    doc->isoOptions().setVolumeID( i18n("DataCD%1").arg(m_dataUntitledCount) );
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);

  return doc;
}


K3bDoc* K3bMainWindow::slotNewDvdDoc()
{
  slotStatusMsg(i18n("Creating new Data DVD Project."));

  K3bDvdDoc* doc = new K3bDvdDoc( this );
  initializeNewDoc( doc );

  m_dvdUntitledCount++;
  QString fileName = i18n("DataDVD%1").arg(m_dvdUntitledCount);
  if( doc->isoOptions().volumeID().isEmpty() )
    doc->isoOptions().setVolumeID( i18n("DataDVD%1").arg(m_dvdUntitledCount) );
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);

  return doc;
}


K3bDoc* K3bMainWindow::slotNewVideoDvdDoc()
{
  slotStatusMsg(i18n("Creating new VideoDVD Project."));

  K3bVideoDvdDoc* doc = new K3bVideoDvdDoc( this );
  initializeNewDoc( doc );

  m_videoDvdUntitledCount++;
  QString fileName = i18n("VideoDVD%1").arg(m_videoDvdUntitledCount);
  if( doc->isoOptions().volumeID().isEmpty() )
    doc->isoOptions().setVolumeID( i18n("VideoDVD%1").arg(m_dvdUntitledCount) );
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);

  KMessageBox::information( this, 
			    i18n("Be aware that you need to provide the complete Video DVD filestructure. "
				 "K3b does not support video transcoding and preparation of video object "
				 "files yet. That means you need to already have the VTS_X_YY.VOB "
				 "and VTS_X_YY.IFO files."),
			    i18n("K3b Video DVD restrictions"),
			    "video_dvd_restrictions" );

  return doc;
}


K3bDoc* K3bMainWindow::slotNewMixedDoc()
{
  slotStatusMsg(i18n("Creating new Mixed Mode CD Project."));

  K3bMixedDoc* doc = new K3bMixedDoc( this );
  initializeNewDoc( doc );

  m_mixedUntitledCount++;
  QString fileName=i18n("MixedCD%1").arg(m_mixedUntitledCount);
  if( doc->dataDoc()->isoOptions().volumeID().isEmpty() )
    doc->dataDoc()->isoOptions().setVolumeID( i18n("MixedCD%1").arg(m_mixedUntitledCount) );
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);

  return doc;
}

K3bDoc* K3bMainWindow::slotNewVcdDoc()
{
  if ( !k3bcore->externalBinManager()->foundBin( "vcdxbuild" ) ) {
    kdDebug() << "(K3bMainWindow) could not find vcdxbuild executable" << endl;
    KMessageBox::information( this,
			      i18n( "Could not find VcdImager executable. "
				    "To create VideoCD's you must install VcdImager >= 0.7.12. "
				    "You can find this on your distribution disks or download "
				    "it from http://www.vcdimager.org" ),
			      i18n( "Information" ) );
  }

  slotStatusMsg(i18n("Creating new Video CD Project."));

  K3bVcdDoc* doc = new K3bVcdDoc( this );
  initializeNewDoc( doc );

  m_vcdUntitledCount++;
  QString fileName=i18n("VideoCD%1").arg(m_vcdUntitledCount);
  if( doc->vcdOptions()->volumeId().isEmpty() )
    doc->vcdOptions()->setVolumeId( i18n("VideoCD%1").arg(m_vcdUntitledCount) );
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);

  return doc;
}


K3bDoc* K3bMainWindow::slotNewMovixDoc()
{
  slotStatusMsg(i18n("Creating new eMovix CD Project."));

  K3bMovixDoc* doc = new K3bMovixDoc( this );
  initializeNewDoc( doc );

  m_movixUntitledCount++;
  QString fileName=i18n("eMovixCD%1").arg(m_movixUntitledCount);
  if( doc->isoOptions().volumeID().isEmpty() )
    doc->isoOptions().setVolumeID( i18n("eMovixCD%1").arg(m_movixUntitledCount) );
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);

  return doc;
}


K3bDoc* K3bMainWindow::slotNewMovixDvdDoc()
{
  slotStatusMsg(i18n("Creating new eMovix DVD Project."));

  K3bMovixDvdDoc* doc = new K3bMovixDvdDoc( this );
  initializeNewDoc( doc );

  m_movixDvdUntitledCount++;
  QString fileName=i18n("eMovixDVD%1").arg(m_movixDvdUntitledCount);
  if( doc->isoOptions().volumeID().isEmpty() )
    doc->isoOptions().setVolumeID( i18n("eMovixDVD%1").arg(m_movixDvdUntitledCount) );
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);

  return doc;
}


void K3bMainWindow::initializeNewDoc( K3bDoc* doc )
{
  doc->newDocument();
  doc->loadDefaultSettings( config() );

  // create the dcop interface
  dcopInterface( doc );
}


K3bProjectInterface* K3bMainWindow::dcopInterface( K3bDoc* doc )
{
  QMap<K3bDoc*, K3bProjectInterface*>::iterator it = d->projectInterfaceMap.find( doc );
  if( it == d->projectInterfaceMap.end() ) {
    K3bProjectInterface* dcopInterface = new K3bProjectInterface( doc );
    d->projectInterfaceMap[doc] = dcopInterface;
    return dcopInterface;
  }
  else
    return it.data();
}


void K3bMainWindow::slotDivxEncoding()
{
  slotStatusMsg(i18n("Creating new video encoding project."));
  K3bDivxView d( this, "divx");
  d.exec();
}


void K3bMainWindow::slotCurrentDocChanged()
{
  // check the doctype
  K3bView* v = activeView();
  if( v ) {
    d->projectManager->setActive( v->doc() );

    //
    // There are two possiblities to plug the project actions:
    // 1. Through KXMLGUIClient::plugActionList
    //    This way we just ask the View for the actionCollection (which it should merge with
    //    the doc's) and plug it into the project menu.
    //    Advantage: easy and clear to handle
    //    Disadvantage: we may only plug all actions at once into one menu
    //
    // 2. Through merging the doc as a KXMLGUIClient
    //    This way every view is a KXMLGUIClient and it's GUI is just merged into the MainWindow's.
    //    Advantage: flexible
    //    Disadvantage: every view needs it's own XML file
    //
    //

    if( factory() ) {
      if( d->lastDoc )
	factory()->removeClient( d->lastDoc->view() );
      factory()->addClient( v );
      d->lastDoc = v->doc();
    }
    else
      kdDebug() << "(K3bMainWindow) ERROR: could not get KXMLGUIFactory instance." << endl;
  }
  else
      d->projectManager->setActive( 0L );
  if( d->projectManager->isEmpty() ) {
    d->documentStack->raiseWidget( d->welcomeWidget );
    slotStateChanged( "state_project_active", KXMLGUIClient::StateReverse );
  }
  else {
    d->documentStack->raiseWidget( d->documentHull );
    // make sure the document header is shown (or not)
    slotViewDocumentHeader();
    slotStateChanged( "state_project_active", KXMLGUIClient::StateNoReverse );
  }
}


void K3bMainWindow::slotEditToolbars()
{
  saveMainWindowSettings( m_config, "main_window_settings" );
  KEditToolbar dlg( factory() );
  connect(&dlg, SIGNAL(newToolbarConfig()), SLOT(slotNewToolBarConfig()));
  dlg.exec();
}


void K3bMainWindow::slotNewToolBarConfig()
{
  applyMainWindowSettings( m_config, "main_window_settings" );
}


bool K3bMainWindow::eject()
{
  QString oldGroup = config()->group();
  config()->setGroup( "General Options" );
  bool eject = !config()->readBoolEntry( "No cd eject", false );
  config()->setGroup( oldGroup );
  return eject;
}


void K3bMainWindow::slotErrorMessage(const QString& message)
{
  KMessageBox::error( this, message );
}


void K3bMainWindow::slotWarningMessage(const QString& message)
{
  KMessageBox::sorry( this, message );
}


void K3bMainWindow::slotBlankCdrw()
{
  K3bBlankingDialog d( this, "blankingdialog" );
  d.exec();
}


void K3bMainWindow::slotFormatDvd()
{
  K3bDvdFormattingDialog d( this );
  d.exec();
}


void K3bMainWindow::slotWriteCdImage()
{
  K3bCdImageWritingDialog d( this );
  d.exec();
}


void K3bMainWindow::slotWriteDvdIsoImage()
{
  K3bIsoImageWritingDialog d( this );
  d.exec();
}


void K3bMainWindow::slotWriteCdImage( const KURL& url )
{
  K3bCdImageWritingDialog d( this );
  d.setImage( url );
  d.exec();
}


void K3bMainWindow::slotProjectAddFiles()
{
  K3bDoc* doc = activeDoc();

  if( doc ) {
    QStringList urls = KFileDialog::getOpenFileNames( ".", i18n("*|All Files"), this,
                                                      i18n("Select Files to Add to Project") );
    if( !urls.isEmpty() )
      doc->addUrls( urls );
  }
  else
    KMessageBox::error( this, i18n("Please create a project before adding files"), i18n("No Active Project"));
}


void K3bMainWindow::slotK3bSetup()
{
  KProcess p;
  p << "kdesu" << "kcmshell k3bsetup2 --lang " + KGlobal::locale()->language();
  if( !p.start( KProcess::DontCare ) )
    KMessageBox::error( 0, i18n("Could not find kdesu to run K3bSetup with root privileges. "
				"Please run it manually as root.") );
}


void K3bMainWindow::slotCdCopy()
{
  K3bCdCopyDialog d( this );
  d.exec();
}


void K3bMainWindow::slotDvdCopy()
{
  K3bDvdCopyDialog d( this );
  d.exec();
}


// void K3bMainWindow::slotVideoDvdCopy()
// {
//   K3bVideoDvdCopyDialog d( this );
//   d.exec();
// }


void K3bMainWindow::slotViewAudioPlayer()
{
  m_audioPlayerDock->changeHideShowState();
  slotCheckDockWidgetStatus();
}


void K3bMainWindow::slotShowDirTreeView()
{
  m_dirTreeDock->changeHideShowState();
  slotCheckDockWidgetStatus();
}


void K3bMainWindow::slotShowContentsView()
{
  m_contentsDock->changeHideShowState();
  slotCheckDockWidgetStatus();
}


void K3bMainWindow::slotShowProjectView()
{
  mainDock->changeHideShowState();
  slotCheckDockWidgetStatus();
}


void K3bMainWindow::slotShowTips()
{
  KTipDialog::showTip( this, QString::null, true );
}


void K3bMainWindow::slotAudioPlayerHidden()
{
  actionViewAudioPlayer->setChecked( false );
}


void K3bMainWindow::slotDirTreeDockHidden()
{
  actionViewDirTreeView->setChecked( false );
}


void K3bMainWindow::slotContentsDockHidden()
{
  actionViewContentsView->setChecked( false );
}


void K3bMainWindow::slotProjectDockHidden()
{
  actionViewProjectView->setChecked( false );
}


void K3bMainWindow::slotCheckDockWidgetStatus()
{
  actionViewAudioPlayer->setChecked( m_audioPlayerDock->isVisible() );
  actionViewContentsView->setChecked( m_contentsDock->isVisible() );
  actionViewProjectView->setChecked( mainDock->isVisible() );
  actionViewDirTreeView->setChecked( m_dirTreeDock->isVisible() );
}


void K3bMainWindow::slotViewDocumentHeader()
{
  if(actionViewDocumentHeader->isChecked()) {
    m_documentHeader->show();
  }
  else {
    m_documentHeader->hide();
  }
}


K3bExternalBinManager* K3bMainWindow::externalBinManager() const
{
  return k3bcore->externalBinManager();
}


K3bDeviceManager* K3bMainWindow::deviceManager() const
{
  return k3bcore->deviceManager();
}


void K3bMainWindow::slotDataImportSession()
{
  if( activeView() ) {
    if( K3bDataView* view = dynamic_cast<K3bDataView*>(activeView()) ) {
      view->importSession();
    }
  }
}


void K3bMainWindow::slotDataClearImportedSession()
{
  if( activeView() ) {
    if( K3bDataView* view = dynamic_cast<K3bDataView*>(activeView()) ) {
      view->clearImportedSession();
    }
  }
}


void K3bMainWindow::slotEditBootImages()
{
  if( activeView() ) {
    if( K3bDataView* view = dynamic_cast<K3bDataView*>(activeView()) ) {
      view->editBootImages();
    }
  }
}


void K3bMainWindow::setProjectsHidable( bool hidable )
{
  if( hidable ) {
  }
  else {
    // TODO: somehow we need to disable the little close button in the dockheader
    //       QDockWindow allows this...

    if( !mainDock->isVisible() )
      slotShowProjectView();
  }
  //  actionViewProjectView->setEnabled( hidable );
}

void K3bMainWindow::slotCheckSystem()
{
  K3bSystemProblemDialog::checkSystem( this );
}


void K3bMainWindow::addUrls( const KURL::List& urls )
{
  if( activeDoc() ) {
    activeDoc()->addUrls( urls );
  }
  else {
    // check if the files are all audio we can handle. If so create an audio project
    bool audio = true;
    QPtrList<K3bPluginFactory> fl = k3bpluginmanager->factories( "AudioDecoder" );
    for( KURL::List::const_iterator it = urls.begin(); it != urls.end(); ++it ) {
      const KURL& url = *it;

      if( QFileInfo(url.path()).isDir() ) {
	audio = false;
	break;
      }

      bool a = false;
      for( QPtrListIterator<K3bPluginFactory> it( fl ); it.current(); ++it ) {
	if( ((K3bAudioDecoderFactory*)it.current())->canDecode( url ) ) {
	  a = true;
	  break;
	}
      }
      if( !a ) {
	audio = a;
	break;
      }
    }

    if( audio )
      slotNewAudioDoc()->addUrls( urls );
    else
      slotNewDataDoc()->addUrls( urls );
  }
}


void K3bMainWindow::slotClearProject()
{
  K3bDoc* doc = k3bprojectmanager->activeDoc();
  if( doc ) {
    if( KMessageBox::questionYesNo( this,
				    i18n("Do you really want to clear the current project?"),
				    i18n("Clear Project"),
				    KStdGuiItem::yes(),
				    KStdGuiItem::no(),
				    "clear_current_project_dontAskAgain" ) == KMessageBox::Yes ) {
      doc->newDocument();
      doc->loadDefaultSettings( config() );
    }
  }
}


void K3bMainWindow::slotThemeChanged()
{
  if( K3bTheme* theme = k3bthememanager->currentTheme() ) {
    d->leftDocPicLabel->setPixmap( theme->pixmap( "k3bprojectview_left_short" ) );
    d->rightDocPicLabel->setPixmap( theme->pixmap( "k3bprojectview_right" ) );
    d->centerDocLabel->setPaletteBackgroundColor( theme->backgroundColor() );
    d->centerDocLabel->setPaletteForegroundColor( theme->foregroundColor() );
  }
}


#include "k3b.moc"
