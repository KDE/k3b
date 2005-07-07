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
#include <qwidgetstack.h>

#include <kdockwidget.h>
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
#include <kio/global.h>

#include <stdlib.h>

// application specific includes
#include "k3b.h"
#include "k3bapplication.h"
#include <k3bglobals.h>
#include "k3bview.h"
#include "k3bdirview.h"
#include <k3baudiodoc.h>
#include "k3baudioview.h"
#include "k3bappdevicemanager.h"
#include "k3baudiotrackdialog.h"
#include "option/k3boptiondialog.h"
#include "k3bprojectburndialog.h"
#include <k3bdatadoc.h>
#include "k3bdataview.h"
#include <k3bdvddoc.h>
#include "k3bdvdview.h"
#include <k3bvideodvddoc.h>
#include "k3bvideodvdview.h"
#include <k3bmixeddoc.h>
#include "k3bmixedview.h"
#include <k3bvcddoc.h>
#include "k3bvcdview.h"
#include <k3bmovixdoc.h>
#include "k3bmovixview.h"
#include <k3bmovixdvddoc.h>
#include "k3bmovixdvdview.h"
#include "misc/k3bblankingdialog.h"
#include "misc/k3bcdimagewritingdialog.h"
#include "misc/k3bisoimagewritingdialog.h"
#include <k3bexternalbinmanager.h>
#include "k3bprojecttabwidget.h"
#include "misc/k3bcdcopydialog.h"
#include "videoEncoding/k3bdivxview.h"
#include "k3btempdirselectionwidget.h"
#include <k3bbusywidget.h>
#include "k3bstatusbarmanager.h"
#include "k3bfiletreecombobox.h"
#include "k3bfiletreeview.h"
#include "k3bsidepanel.h"
#include "k3bstdguiitems.h"
#include "misc/k3bdvdformattingdialog.h"
#include "misc/k3bdvdcopydialog.h"
//#include "dvdcopy/k3bvideodvdcopydialog.h"
#include "k3bprojectmanager.h"
#include "k3bwelcomewidget.h"
#include <k3bpluginmanager.h>
#include <k3bplugin.h>
#include "k3bsystemproblemdialog.h"
#include <k3baudiodecoder.h>
#include <k3bthememanager.h>
#include <k3biso9660.h>
#include <k3bcuefileparser.h>
#include <k3bdeviceselectiondialog.h>
#include <k3bjob.h>
#include <k3bsignalwaiter.h>


class K3bMainWindow::Private
{
public:
  K3bDoc* lastDoc;

  QWidgetStack* documentStack;
  K3bWelcomeWidget* welcomeWidget;
  QWidget* documentHull;

  QLabel* leftDocPicLabel;
  QLabel* centerDocLabel;
  QLabel* rightDocPicLabel;
};


K3bMainWindow::K3bMainWindow()
  : DockMainWindow(0,"K3bMainwindow")
{
  //setup splitter behavior
  manager()->setSplitterHighResolution(true);
  manager()->setSplitterOpaqueResize(true);
  manager()->setSplitterKeepSize(true);

  d = new Private;
  d->lastDoc = 0;

  setPlainCaption( i18n("K3b - The CD and DVD Kreator") );

  m_config = kapp->config();

  ///////////////////////////////////////////////////////////////////
  // call inits to invoke all other construction parts
  initActions();
  initView();
  initStatusBar();
  createGUI(0L);

  // we need the actions for the welcomewidget
  d->welcomeWidget->loadConfig( config() );

  // fill the tabs action menu
  m_documentTab->insertAction( actionFileSave );
  m_documentTab->insertAction( actionFileSaveAs );
  m_documentTab->insertAction( actionFileClose );

  // /////////////////////////////////////////////////////////////////
  // disable actions at startup
  slotStateChanged( "state_project_active", KXMLGUIClient::StateReverse );

  // connect to the busy signals
  connect( k3bcore, SIGNAL(busyInfoRequested(const QString&)), this, SLOT(showBusyInfo(const QString&)) );
  connect( k3bcore, SIGNAL(busyFinishRequested()), this, SLOT(endBusy()) );
  connect( k3bappcore->projectManager(), SIGNAL(newProject(K3bDoc*)), this, SLOT(createClient(K3bDoc*)) );
  connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );

  // FIXME: now make sure the welcome screen is displayed completely
  resize( 780, 550 );
//   getMainDockWidget()->resize( getMainDockWidget()->size().expandedTo( d->welcomeWidget->sizeHint() ) );
//   m_dirTreeDock->resize( QSize( m_dirTreeDock->sizeHint().width(), m_dirTreeDock->height() ) );

  readOptions();
}

K3bMainWindow::~K3bMainWindow()
{
  delete mainDock;
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
  // merge in the device actions from the device manager
  // operator+= is deprecated but I know no other way to do this. Why does the KDE app framework
  // need to have all actions in the mainwindow's actioncollection anyway (or am I just to stupid to
  // see the correct solution?)
  *actionCollection() += *k3bappcore->appDeviceManager()->actionCollection();

  actionFileOpen = KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
  actionFileOpenRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const KURL&)), actionCollection());
  actionFileSave = KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
  actionFileSaveAs = KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  actionFileSaveAll = new KAction( i18n("Save All"), "save_all", 0, this, SLOT(slotFileSaveAll()), 
				   actionCollection(), "file_save_all" );
  actionFileClose = KStdAction::close(this, SLOT(slotFileClose()), actionCollection());
  actionFileCloseAll = new KAction( i18n("Close All"), 0, 0, this, SLOT(slotFileCloseAll()), 
				    actionCollection(), "file_close_all" );
  actionFileQuit = KStdAction::quit(this, SLOT(slotFileQuit()), actionCollection());
  actionViewStatusBar = KStdAction::showStatusbar(this, SLOT(slotViewStatusBar()), actionCollection());
  actionSettingsConfigure = KStdAction::preferences(this, SLOT(slotSettingsConfigure()), actionCollection() );

  // the tip action
  (void)KStdAction::tipOfDay(this, SLOT(slotShowTips()), actionCollection() );
  (void)KStdAction::keyBindings( this, SLOT( slotConfigureKeys() ), actionCollection() );

  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
  setStandardToolBarMenuEnabled(true);

  actionFileNewMenu = new KActionMenu( i18n("&New Project"), "filenew", actionCollection(), "file_new" );
//   KActionMenu* actionMenuCD = new KActionMenu( i18n("&CD Project"), "filenew", actionCollection(), "file_new_cd" );
//   KActionMenu* actionMenuDVD = new KActionMenu( i18n("&DVD Project"), "filenew", actionCollection(), "file_new_dvd" );
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


//   actionFileNewMenu->insert( actionMenuCD );
//   actionFileNewMenu->insert( actionMenuDVD );
  actionFileNewMenu->setDelayed( false );
  actionFileNewMenu->insert( actionFileNewData );
  actionFileNewMenu->insert( actionFileNewDvd );
  actionFileNewMenu->insert( new KActionSeparator( this ) );
  actionFileNewMenu->insert( actionFileNewAudio );
  actionFileNewMenu->insert( new KActionSeparator( this ) );
  actionFileNewMenu->insert( actionFileNewMixed );
  actionFileNewMenu->insert( new KActionSeparator( this ) );
  actionFileNewMenu->insert( actionFileNewVcd );
  actionFileNewMenu->insert( actionFileNewVideoDvd );
  actionFileNewMenu->insert( new KActionSeparator( this ) );
  actionFileNewMenu->insert( actionFileNewMovix );
  actionFileNewMenu->insert( actionFileNewMovixDvd );





  actionProjectAddFiles = new KAction( i18n("&Add Files..."), "filenew", 0, this, SLOT(slotProjectAddFiles()),
				       actionCollection(), "project_add_files");

  (void)new KAction( i18n("&Clear Project"), QApplication::reverseLayout() ? "clear_left" : "locationbar_erase", 0, this, SLOT(slotClearProject()),
		     actionCollection(), "project_clear_project" );

  actionViewDirTreeView = new KToggleAction(i18n("Show Directories"), 0, this, SLOT(slotShowDirTreeView()),
					    actionCollection(), "view_dir_tree");

  actionViewContentsView = new KToggleAction(i18n("Show Contents"), 0, this, SLOT(slotShowContentsView()),
					     actionCollection(), "view_contents");

  actionViewDocumentHeader = new KToggleAction(i18n("Show Document Header"), 0, this, SLOT(slotViewDocumentHeader()),
					       actionCollection(), "view_document_header");

  actionToolsBlankCdrw = new KAction( i18n("&Erase CD-RW..."), "cdrwblank", 0, this, SLOT(slotBlankCdrw()),
				      actionCollection(), "tools_blank_cdrw" );
  /*KAction* actionToolsFormatDVD = */(void)new KAction( i18n("&Format DVD±RW..."), "cdrwblank", 0, this, SLOT(slotFormatDvd()),
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



  actionFileNewMenu->setToolTip(i18n("Creates a new project"));
  actionFileNewData->setToolTip( i18n("Creates a new data CD project") );
  actionFileNewAudio->setToolTip( i18n("Creates a new audio CD project") );
  actionFileNewMovixDvd->setToolTip( i18n("Creates a new eMovix DVD project") );
  actionFileNewDvd->setToolTip( i18n("Creates a new data DVD project") );
  actionFileNewMovix->setToolTip( i18n("Creates a new eMovix CD project") );
  actionFileNewVcd->setToolTip( i18n("Creates a new Video CD project") );
  actionToolsBlankCdrw->setToolTip( i18n("Opens CD-blanking dialog") );
  actionCdCopy->setToolTip( i18n("Open the CD Copy dialog") );
  actionToolsWriteCdImage->setToolTip( i18n("Write an Iso9660, cue/bin, or cdrecord clone image") );
  actionToolsDvdCopy->setToolTip( i18n("Open the DVD Copy dialog") );
  //  actionToolsVideoDvdCopy->setToolTip( i18n("Open the VideoDVD Copy dialog") );
  actionFileOpen->setToolTip(i18n("Opens an existing project"));
  actionFileOpenRecent->setToolTip(i18n("Opens a recently used file"));
  actionFileSave->setToolTip(i18n("Saves the actual project"));
  actionFileSaveAs->setToolTip(i18n("Saves the active project as..."));
  actionFileClose->setToolTip(i18n("Closes the active project"));
  actionFileQuit->setToolTip(i18n("Quits the application"));

  // make sure the tooltips are used for the menu
  actionCollection()->setHighlightingEnabled( true );
}



const QPtrList<K3bDoc>& K3bMainWindow::projects() const
{
  return k3bappcore->projectManager()->projects();
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
  mainDock->setDockSite( KDockWidget::DockCorner );
  mainDock->setEnableDocking( KDockWidget::DockNone );
  setView( mainDock );
  setMainDockWidget( mainDock );

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

  documentHullLayout->addWidget( m_documentHeader, 0, 0 );
  documentHullLayout->addWidget( m_documentTab, 1, 0 );

  connect( m_documentTab, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotCurrentDocChanged()) );

  d->welcomeWidget = new K3bWelcomeWidget( this, d->documentStack );

  d->documentStack->addWidget( d->welcomeWidget );
  d->documentStack->raiseWidget( d->welcomeWidget );
  // ---------------------------------------------------------------------------------------------

  // --- Directory Dock --------------------------------------------------------------------------
  m_dirTreeDock = createDockWidget( "directory_tree", SmallIcon("folder"), 0,
				    kapp->makeStdCaption( i18n("Sidepanel") ), i18n("Sidepanel") );
  m_dirTreeDock->setEnableDocking( KDockWidget::DockCorner );

  K3bFileTreeView* sidePanel = new K3bFileTreeView( m_dirTreeDock );
  //K3bSidePanel* sidePanel = new K3bSidePanel( this, m_dirTreeDock, "sidePanel" );

  m_dirTreeDock->setWidget( sidePanel );
  m_dirTreeDock->manualDock( mainDock, KDockWidget::DockTop, 4000 );
  connect( m_dirTreeDock, SIGNAL(iMBeingClosed()), this, SLOT(slotDirTreeDockHidden()) );
  connect( m_dirTreeDock, SIGNAL(hasUndocked()), this, SLOT(slotDirTreeDockHidden()) );
  // ---------------------------------------------------------------------------------------------

  // --- Contents Dock ---------------------------------------------------------------------------
  m_contentsDock = createDockWidget( "contents_view", SmallIcon("idea"), 0,
			      kapp->makeStdCaption( i18n("Contents View") ), i18n("Contents View") );
  m_contentsDock->setEnableDocking( KDockWidget::DockCorner );
  m_dirView = new K3bDirView( sidePanel/*->fileTreeView()*/, m_contentsDock );
  m_contentsDock->setWidget( m_dirView );
  m_contentsDock->manualDock( m_dirTreeDock, KDockWidget::DockRight, 2000 );

  connect( m_contentsDock, SIGNAL(iMBeingClosed()), this, SLOT(slotContentsDockHidden()) );
  connect( m_contentsDock, SIGNAL(hasUndocked()), this, SLOT(slotContentsDockHidden()) );
  // ---------------------------------------------------------------------------------------------

  // --- filetreecombobox-toolbar ----------------------------------------------------------------
  K3bFileTreeComboBox* m_fileTreeComboBox = new K3bFileTreeComboBox( 0 );
  connect( m_fileTreeComboBox, SIGNAL(urlExecuted(const KURL&)), m_dirView, SLOT(showUrl(const KURL& )) );
  connect( m_fileTreeComboBox, SIGNAL(deviceExecuted(K3bDevice::Device*)), m_dirView,
	   SLOT(showDevice(K3bDevice::Device* )) );
  connect( m_dirView, SIGNAL(urlEntered(const KURL&)), m_fileTreeComboBox, SLOT(setUrl(const KURL&)) );

  KWidgetAction* fileTreeComboAction = new KWidgetAction( m_fileTreeComboBox,
							  i18n("&Quick Dir Selector"),
							  0, 0, 0,
							  actionCollection(), "quick_dir_selector" );
  fileTreeComboAction->setAutoSized(true);
  (void)new KAction( i18n("Go"), "key_enter", 0, m_fileTreeComboBox, SLOT(slotGoUrl()), actionCollection(), "go_url" );
  // ---------------------------------------------------------------------------------------------
}


void K3bMainWindow::createClient( K3bDoc* doc )
{
  // create the proper K3bView (maybe we should put this into some other class like K3bProjectManager)
  K3bView* view = 0;
  switch( doc->type() ) {
  case K3bDoc::AUDIO:
    view = new K3bAudioView( static_cast<K3bAudioDoc*>(doc), m_documentTab );
    break;
  case K3bDoc::DATA:
    view = new K3bDataView( static_cast<K3bDataDoc*>(doc), m_documentTab );
    break; 
  case K3bDoc::MIXED:
    {
      K3bMixedDoc* mixedDoc = static_cast<K3bMixedDoc*>(doc);
      view = new K3bMixedView( mixedDoc, m_documentTab );
      mixedDoc->dataDoc()->setView( view );
      mixedDoc->audioDoc()->setView( view );
      break; 
    }
  case K3bDoc::VCD:
    view = new K3bVcdView( static_cast<K3bVcdDoc*>(doc), m_documentTab );
    break; 
  case K3bDoc::MOVIX:
    view = new K3bMovixView( static_cast<K3bMovixDoc*>(doc), m_documentTab );
    break;
  case K3bDoc::MOVIX_DVD:
    view = new K3bMovixDvdView( static_cast<K3bMovixDvdDoc*>(doc), m_documentTab );
    break;
  case K3bDoc::DVD:
    view = new K3bDvdView( static_cast<K3bDvdDoc*>(doc), m_documentTab );
    break;
  case K3bDoc::VIDEODVD:
    view = new K3bVideoDvdView( static_cast<K3bVideoDvdDoc*>(doc), m_documentTab );
    break;
  }

  doc->setView( view );
  view->setCaption( doc->URL().fileName() );

  m_documentTab->insertTab( doc );
  m_documentTab->showPage( view );

  slotCurrentDocChanged();
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

  //
  // First we check if this is an iso image in case someone wants to open one this way
  //
  if( !isCdDvdImageAndIfSoOpenDialog( url ) ) {

    // see if it's an audio cue file
    K3bCueFileParser parser( url.path() );
    if( parser.isValid() && parser.toc().contentType() == K3bDevice::AUDIO ) {
      K3bDoc* doc = k3bappcore->projectManager()->createProject( K3bDoc::AUDIO );
      doc->addUrl( url );
      return doc;
    }
    else {
      // check, if document already open. If yes, set the focus to the first view
      K3bDoc* doc = k3bappcore->projectManager()->findByUrl( url );
      if( doc ) {
	doc->view()->setFocus();
	return doc;
      }

      doc = k3bappcore->projectManager()->openProject( url );

      if( doc == 0 ) {
	KMessageBox::error (this,i18n("Could not open document!"), i18n("Error!"));
	return 0;
      }

      actionFileOpenRecent->addURL(url);

      return doc;
    }
  }
  else
    return 0;
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

  k3bcore->saveSettings( config() );

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

  m_dirView->readConfig( config() );

  slotViewDocumentHeader();
  slotCheckDockWidgetStatus();

  slotThemeChanged();
}


void K3bMainWindow::saveProperties( KConfig* c )
{
  // 1. put saved projects in the config
  // 2. save every modified project in  "~/.kde/share/apps/k3b/sessions/" + KApp->sessionId()
  // 3. save the url of the project (might be something like "AudioCD1") in the config
  // 4. save the status of every project (modified/saved)

  QString saveDir = KGlobal::dirs()->saveLocation( "appdata", "sessions/" + qApp->sessionId() + "/", true );

  const QPtrList<K3bDoc>& docs = k3bappcore->projectManager()->projects();
  c->writeEntry( "Number of projects", docs.count() );

  int cnt = 1;
  for( QPtrListIterator<K3bDoc> it( docs ); *it; ++it ) {
    // the "name" of the project (or the original url if isSaved())
    c->writePathEntry( QString("%1 url").arg(cnt), (*it)->URL().url() );

    // is the doc modified
    c->writeEntry( QString("%1 modified").arg(cnt), (*it)->isModified() );

    // has the doc already been saved?
    c->writeEntry( QString("%1 saved").arg(cnt), (*it)->isSaved() );

    // where does the session management save it? If it's not modified and saved this is
    // the same as the url
    KURL saveUrl = (*it)->URL();
    if( !(*it)->isSaved() || (*it)->isModified() )
      saveUrl = KURL::fromPathOrURL( saveDir + QString::number(cnt) );
    c->writePathEntry( QString("%1 saveurl").arg(cnt), saveUrl.url() );

    // finally save it
    k3bappcore->projectManager()->saveProject( *it, saveUrl );

    ++cnt;
  }
}


// FIXME:move this to K3bProjectManager
void K3bMainWindow::readProperties( KConfig* c )
{
  // FIXME: do not delete the files here. rather do it when the app is exited normally
  //        since that's when we can be sure we never need the session stuff again.

  // 1. read all projects from the config
  // 2. simply open all of them
  // 3. reset the saved urls and the modified state
  // 4. delete "~/.kde/share/apps/k3b/sessions/" + KApp->sessionId()

  QString saveDir = KGlobal::dirs()->saveLocation( "appdata", "sessions/" + qApp->sessionId() + "/", true );
  kdDebug() << "(K3bMainWindow::readProperties) saveDir: " << saveDir << endl;

  int cnt = c->readNumEntry( "Number of projects", 0 );
  kdDebug() << "(K3bMainWindow::readProperties) num: " << cnt << endl;

  for( int i = 1; i <= cnt; ++i ) {
    // in this case the constructor works since we saved as url()
    KURL url = c->readPathEntry( QString("%1 url").arg(i) );

    bool modified = c->readBoolEntry( QString("%1 modified").arg(i) );

    bool saved = c->readBoolEntry( QString("%1 saved").arg(i) );

    KURL saveUrl = c->readPathEntry( QString("%1 saveurl").arg(i) );

    // now load the project
    if( K3bDoc* doc = k3bappcore->projectManager()->openProject( saveUrl ) ) {

      // reset the url
      doc->setURL( url );
      doc->setModified( modified );
      doc->setSaved( saved );
    }
    else
      kdDebug() << "(K3bMainWindow) could not open session saved doc " << url.path() << endl;

    // remove the temp file
    if( !saved || modified )
      QFile::remove( saveUrl.path() );
  }

  // and now remove the temp dir
  KIO::del( KURL::fromPathOrURL(saveDir), false, false );
}


bool K3bMainWindow::queryClose()
{
  //
  // Check if a job is currently running
  // For now K3b only allows for one major job at a time which means that we only need to cancel
  // this one job.
  //
  if( k3bcore->jobsRunning() ) {

    // pitty, but I see no possibility to make this work. It always crashes because of the event
    // management thing mentioned below. So until I find a solution K3b simply will refuse to close
    // while a job i running
    return false;

//     kdDebug() << "(K3bMainWindow::queryClose) jobs running." << endl;
//     K3bJob* job = k3bcore->runningJobs().getFirst();
    
//     // now search for the major job (to be on the safe side although for now no subjobs register with the k3bcore)
//     K3bJobHandler* jh = job->jobHandler();
//     while( jh->isJob() ) {
//       job = static_cast<K3bJob*>( jh );
//       jh = job->jobHandler();
//     }

//     kdDebug() << "(K3bMainWindow::queryClose) main job found: " << job->jobDescription() << endl;

//     // now job is the major job and jh should be a widget
//     QWidget* progressDialog = dynamic_cast<QWidget*>( jh );

//     kdDebug() << "(K3bMainWindow::queryClose) job active: " << job->active() << endl;

//     // now ask the user if he/she really wants to cancel this job
//     if( job->active() ) {
//       if( KMessageBox::questionYesNo( progressDialog ? progressDialog : this,
// 				      i18n("Do you really want to cancel?"), 
// 				      i18n("Cancel") ) == KMessageBox::Yes ) {
// 	// cancel the job
// 	kdDebug() << "(K3bMainWindow::queryClose) cancelling job." << endl;
// 	job->cancel();

// 	// wait for the job to finish
// 	kdDebug() << "(K3bMainWindow::queryClose) waiting for job to finish." << endl;
// 	K3bSignalWaiter::waitForJob( job );

// 	// close the progress dialog
// 	if( progressDialog ) {
// 	  kdDebug() << "(K3bMainWindow::queryClose) closing progress dialog." << endl;
// 	  progressDialog->close();
// 	  //
// 	  // now here we have the problem that due to the whole Qt event thing the exec call (or
// 	  // in this case most likely the startJob call) does not return until we leave this method.
// 	  // That means that the progress dialog might be deleted by it's parent below (when we 
// 	  // close docs) before it is deleted by the creator (most likely a projectburndialog).
// 	  // That would result in a double deletion and thus a crash.
// 	  // So we just reparent the dialog to 0 here so it's (former) parent won't delete it.
// 	  //
// 	  progressDialog->reparent( 0, QPoint(0,0) );
// 	}

// 	kdDebug() << "(K3bMainWindow::queryClose) job cleanup done." << endl;
//       }
//       else
// 	return false;
//     }
  }

  //
  // if we are closed by the session manager everything is fine since we store the
  // current state in saveProperties
  //
  if( kapp->sessionSaving() ) 
    return true;

  // FIXME: do not close the docs here. Just ask for them to be saved and return false
  //        if the user chose cancel for some doc

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
  if( !doc->isModified() )
    return true;

  m_config->setGroup( "General Options" );
  if( !m_config->readBoolEntry( "ask_for_saving_changes_on_exit", true ) )
    return true;

  switch ( KMessageBox::warningYesNoCancel( this, 
					    i18n("%1 has unsaved data.").arg( doc->URL().fileName() ),
					    i18n("Closing Project"), 
					    KGuiItem( i18n("&Save"), "filesave" ),
					    KGuiItem( i18n("&Discard"), "editshred" ) ) )
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
  // TODO: call this in K3bApplication somewhere
  saveOptions();
  return true;
}



/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////


void K3bMainWindow::slotFileOpen()
{
  slotStatusMsg(i18n("Opening file..."));

  KURL::List urls = KFileDialog::getOpenURLs( QString::null,
					      i18n("*.k3b|K3b Projects"),
					      this,
					      i18n("Open Files") );
  for( KURL::List::iterator it = urls.begin(); it != urls.end(); ++it ) {
    openDocument( *it );
    actionFileOpenRecent->addURL( *it );
  }
}

void K3bMainWindow::slotFileOpenRecent(const KURL& url)
{
  slotStatusMsg(i18n("Opening file..."));

  openDocument(url);
}


void K3bMainWindow::slotFileSaveAll()
{
  for( QPtrListIterator<K3bDoc> it( k3bappcore->projectManager()->projects() );
       *it; ++it ) {
    fileSave( *it );
  }
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
    if( !doc->isSaved() )
      fileSaveAs( doc );
    else if( !k3bappcore->projectManager()->saveProject( doc, doc->URL()) )
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

    QString file = KFileDialog::getSaveFileName(QDir::currentDirPath(),
					       i18n("*.k3b|K3b Projects"), this, i18n("Save As"));


    if( !file.isEmpty() ) {

      // default to ending ".k3b"
      if( file.mid( file.findRev('.')+1 ) != "k3b" ) {
	if( file[ file.length()-1 ] != '.' )
	  file += ".";
	file += "k3b";
      }

      if( !QFile::exists(file) ||
	  ( QFile::exists(file) &&
	    KMessageBox::warningContinueCancel( this, i18n("Do you want to overwrite %1?").arg(file), i18n("File Exists"), i18n("Overwrite") )
	    == KMessageBox::Continue ) ) {

        KURL url;
        url.setPath(file);
	if( !k3bappcore->projectManager()->saveProject( doc, url ) ) {
	  KMessageBox::error (this,i18n("Could not save the current document!"), i18n("I/O Error"));
	  return;
	}

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


void K3bMainWindow::slotFileCloseAll()
{
  while( K3bView* pView = activeView() ) {
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
      factory()->removeClient( static_cast<K3bView*>(d->lastDoc->view()) );
      d->lastDoc = 0;
    }
  }

  // remove the view from the project tab
  m_documentTab->removePage( doc->view() );

  // remove the project from the manager
  k3bappcore->projectManager()->removeProject( doc );

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
  slotStatusMsg(i18n("Creating new Audio CD Project."));

  K3bDoc* doc = k3bappcore->projectManager()->createProject( K3bDoc::AUDIO );

  return doc;
}

K3bDoc* K3bMainWindow::slotNewDataDoc()
{
  slotStatusMsg(i18n("Creating new Data CD Project."));

  K3bDoc* doc = k3bappcore->projectManager()->createProject( K3bDoc::DATA );

  return doc;
}


K3bDoc* K3bMainWindow::slotNewDvdDoc()
{
  slotStatusMsg(i18n("Creating new Data DVD Project."));

  K3bDoc* doc = k3bappcore->projectManager()->createProject( K3bDoc::DVD );

  return doc;
}


K3bDoc* K3bMainWindow::slotNewVideoDvdDoc()
{
  slotStatusMsg(i18n("Creating new VideoDVD Project."));

  K3bDoc* doc = k3bappcore->projectManager()->createProject( K3bDoc::VIDEODVD );

  return doc;
}


K3bDoc* K3bMainWindow::slotNewMixedDoc()
{
  slotStatusMsg(i18n("Creating new Mixed Mode CD Project."));

  K3bDoc* doc = k3bappcore->projectManager()->createProject( K3bDoc::MIXED );

  return doc;
}

K3bDoc* K3bMainWindow::slotNewVcdDoc()
{
  slotStatusMsg(i18n("Creating new Video CD Project."));

  K3bDoc* doc = k3bappcore->projectManager()->createProject( K3bDoc::VCD );

  return doc;
}


K3bDoc* K3bMainWindow::slotNewMovixDoc()
{
  slotStatusMsg(i18n("Creating new eMovix CD Project."));

  K3bDoc* doc = k3bappcore->projectManager()->createProject( K3bDoc::MOVIX );

  return doc;
}


K3bDoc* K3bMainWindow::slotNewMovixDvdDoc()
{
  slotStatusMsg(i18n("Creating new eMovix DVD Project."));

  K3bDoc* doc = k3bappcore->projectManager()->createProject( K3bDoc::MOVIX_DVD );

  return doc;
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
    k3bappcore->projectManager()->setActive( v->doc() );

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
	factory()->removeClient( static_cast<K3bView*>(d->lastDoc->view()) );
      factory()->addClient( v );
      d->lastDoc = v->doc();
    }
    else
      kdDebug() << "(K3bMainWindow) ERROR: could not get KXMLGUIFactory instance." << endl;
  }
  else
      k3bappcore->projectManager()->setActive( 0L );
  if( k3bappcore->projectManager()->isEmpty() ) {
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
  KConfigGroup c( config(), "General Options" );
  return !c.readBoolEntry( "No cd eject", false );
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
  d.exec(false);
}


void K3bMainWindow::slotFormatDvd()
{
  K3bDvdFormattingDialog d( this );
  d.exec(false);
}


void K3bMainWindow::slotWriteCdImage()
{
  K3bCdImageWritingDialog d( this );
  d.exec(false);
}


void K3bMainWindow::slotWriteDvdIsoImage()
{
  K3bIsoImageWritingDialog d( this );
  d.exec(false);
}


void K3bMainWindow::slotWriteDvdIsoImage( const KURL& url )
{
  K3bIsoImageWritingDialog d( this );
  d.setImage( url );
  d.exec(false);
}


void K3bMainWindow::slotWriteCdImage( const KURL& url )
{
  K3bCdImageWritingDialog d( this );
  d.setImage( url );
  d.exec(false);
}


void K3bMainWindow::slotProjectAddFiles()
{
  K3bView* view = activeView();

  if( view ) {
    QStringList files = KFileDialog::getOpenFileNames( ".", 
						       "*|All Files", 
						       this, 
						       i18n("Select Files to Add to Project") );

    KURL::List urls;
    for( QStringList::ConstIterator it = files.begin();
         it != files.end(); it++ ) {
      KURL url;
      url.setPath(*it);
      urls.append( url );
    }

    if( !urls.isEmpty() )
      view->addUrls( urls );
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
  d.exec(false);
}


void K3bMainWindow::slotDvdCopy()
{
  K3bDvdCopyDialog d( this );
  d.exec(false);
}


// void K3bMainWindow::slotVideoDvdCopy()
// {
//   K3bVideoDvdCopyDialog d( this );
//   d.exec();
// }


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


void K3bMainWindow::slotShowTips()
{
  KTipDialog::showTip( this, QString::null, true );
}


void K3bMainWindow::slotDirTreeDockHidden()
{
  actionViewDirTreeView->setChecked( false );
}


void K3bMainWindow::slotContentsDockHidden()
{
  actionViewContentsView->setChecked( false );
}


void K3bMainWindow::slotCheckDockWidgetStatus()
{
  actionViewContentsView->setChecked( m_contentsDock->isVisible() );
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


K3bDevice::DeviceManager* K3bMainWindow::deviceManager() const
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


void K3bMainWindow::slotCheckSystem()
{
  K3bSystemProblemDialog::checkSystem( this );
}


void K3bMainWindow::addUrls( const KURL::List& urls )
{
  //
  // A lot of users try to write an iso image with a data project
  //
  if( urls.count() == 1 ) {
    if( isCdDvdImageAndIfSoOpenDialog( urls.first() ) )
      return;
  }

  if( K3bView* view = activeView() ) {
    view->addUrls( urls );
  }
  else {
    // check if the files are all audio we can handle. If so create an audio project
    bool audio = true;
    QPtrList<K3bPlugin> fl = k3bcore->pluginManager()->plugins( "AudioDecoder" );
    for( KURL::List::const_iterator it = urls.begin(); it != urls.end(); ++it ) {
      const KURL& url = *it;

      if( QFileInfo(url.path()).isDir() ) {
	audio = false;
	break;
      }

      bool a = false;
      for( QPtrListIterator<K3bPlugin> it( fl ); it.current(); ++it ) {
	if( static_cast<K3bAudioDecoderFactory*>(it.current())->canDecode( url ) ) {
	  a = true;
	  break;
	}
      }
      if( !a ) {
	audio = a;
	break;
      }
    }

    if( !audio && urls.count() == 1 ) {
      // see if it's an audio cue file
      K3bCueFileParser parser( urls.first().path() );
      if( parser.isValid() && parser.toc().contentType() == K3bDevice::AUDIO ) {
	audio = true;
      }
    }

    if( audio )
      static_cast<K3bView*>(slotNewAudioDoc()->view())->addUrls( urls );
    else
      static_cast<K3bView*>(slotNewDataDoc()->view())->addUrls( urls );
  }
}


void K3bMainWindow::slotClearProject()
{
  K3bDoc* doc = k3bappcore->projectManager()->activeDoc();
  if( doc ) {
    if( KMessageBox::warningContinueCancel( this,
				    i18n("Do you really want to clear the current project?"),
				    i18n("Clear Project"),
				    i18n("Clear"),
				    "clear_current_project_dontAskAgain" ) == KMessageBox::Continue ) {
      doc->newDocument();
      k3bappcore->projectManager()->loadDefaults( doc );
    }
  }
}


void K3bMainWindow::slotThemeChanged()
{
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    d->leftDocPicLabel->setPixmap( theme->pixmap( K3bTheme::PROJECT_LEFT ) );
    d->rightDocPicLabel->setPixmap( theme->pixmap( K3bTheme::PROJECT_RIGHT ) );
    d->centerDocLabel->setPaletteBackgroundColor( theme->backgroundColor() );
    d->centerDocLabel->setPaletteForegroundColor( theme->foregroundColor() );
  }
}


bool K3bMainWindow::isCdDvdImageAndIfSoOpenDialog( const KURL& url )
{
  K3bIso9660 iso( url.path() );
  if( iso.open() ) {
    iso.close();
    // very rough dvd image size test
    if( K3b::filesize( url ) > 1000*1024*1024 )
      slotWriteDvdIsoImage( url );
    else
      slotWriteCdImage( url );

    return true;
  }
  else
    return false;
}

#include "k3b.moc"
