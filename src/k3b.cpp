/***************************************************************************
                          k3b.cpp  -  description
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

// include files for QT
#include <qdir.h>
#include <qfile.h>
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
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <kglobalsettings.h>
#include <kdialog.h>
#include <kedittoolbar.h>
#include <ksystemtray.h>
#include <kaboutdata.h>
#include <kmessagebox.h>

#include <stdlib.h>

// application specific includes
#include "k3b.h"
#include "tools/k3bglobals.h"
#include "k3bview.h"
#include "k3bdirview.h"
#include "audio/k3baudiodoc.h"
#include "audio/k3baudioview.h"
#include "device/k3bdevicemanager.h"
#include "audio/k3baudiotrackdialog.h"
#include "option/k3boptiondialog.h"
#include "k3bprojectburndialog.h"
#include "data/k3bdatadoc.h"
#include "data/k3bdataview.h"
#include "mixed/k3bmixeddoc.h"
#include "vcd/k3bvcddoc.h"
#include "movix/k3bmovixdoc.h"
#include "k3bblankingdialog.h"
#include "data/k3bisoimagewritingdialog.h"
#include "data/k3bbinimagewritingdialog.h"
#include "tools/k3bexternalbinmanager.h"
#include "tools/k3bdefaultexternalprograms.h"
#include "k3bprojecttabwidget.h"
#include "rip/songdb/k3bsongmanager.h"
#include "k3baudioplayer.h"
#include "cdcopy/k3bcdcopydialog.h"
#include "videoEncoding/k3bdivxview.h"
#include "k3btempdirselectionwidget.h"
#include "tools/k3bbusywidget.h"
#include "k3bstatusbarmanager.h"



K3bMainWindow* k3bMain()
{
  K3bMainWindow* _app = dynamic_cast<K3bMainWindow*>( kapp->mainWidget() );
  if( !_app ) {
    kdDebug() << "No K3bMainWindow found!" << endl;
    exit(1);
  }
  return _app;
}



K3bMainWindow::K3bMainWindow()
  : KDockMainWindow(0,"K3b")
{
  setPlainCaption( i18n("K3b - The CD Kreator") );

  m_config = kapp->config();
  m_audioUntitledCount = 0;
  m_dataUntitledCount = 0;
  m_mixedUntitledCount = 0;
  m_vcdUntitledCount = 0;
  m_movixUntitledCount = 0;

  pDocList = new QPtrList<K3bDoc>();
  pDocList->setAutoDelete(true);


  // the system tray widget
  //  m_systemTray = new KSystemTray( this );

  //m_systemTray->show();


  ///////////////////////////////////////////////////////////////////
  // call inits to invoke all other construction parts
  initStatusBar();
  initActions();

  ///////////////////////////////////////////////////////////////////
  // disable actions at startup
  actionFileSave->setEnabled(false);
  actionFileSaveAs->setEnabled(false);
  actionFileBurn->setEnabled( false );
  actionProjectAddFiles->setEnabled( false );

  actionDataImportSession->setEnabled( false );
  actionDataClearImportedSession->setEnabled( false );
  //  actionDataEditBootImages->setEnabled(false);
  toolBar("dataToolBar")->hide();

  m_optionDialog = 0;

  // since the icons are not that good activate the text on the toolbar
  //  toolBar()->setIconText( KToolBar::IconTextRight );
}

K3bMainWindow::~K3bMainWindow()
{
  delete pDocList;
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
  actionViewToolBar = KStdAction::showToolbar(this, SLOT(slotViewToolBar()), actionCollection());
  actionViewStatusBar = KStdAction::showStatusbar(this, SLOT(slotViewStatusBar()), actionCollection());
  actionSettingsConfigure = KStdAction::preferences(this, SLOT(slotSettingsConfigure()), actionCollection() );

  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());

  actionFileBurn = new KAction( i18n("&Burn..."), "cdwriter_unmount", CTRL + Key_B, this, SLOT(slotFileBurn()),
			  actionCollection(), "file_burn");

  actionFileNewMenu = new KActionMenu( i18n("&New Project"), "filenew", actionCollection(), "file_new" );
  actionFileNewAudio = new KAction(i18n("New &Audio Project"), "sound", 0, this, SLOT(slotNewAudioDoc()),
			     actionCollection(), "file_new_audio");
  actionFileNewData = new KAction(i18n("New &Data Project"),"tar", 0, this, SLOT(slotNewDataDoc()),
			    actionCollection(), "file_new_data");
  actionFileNewMixed = new KAction(i18n("New &Mixed Mode Project"),"tar", 0, this, SLOT(slotNewMixedDoc()),
				   actionCollection(), "file_new_mixed");
  actionFileNewVcd = new KAction(i18n("New &Video Project"),"video", 0, this, SLOT(slotNewVcdDoc()),
				   actionCollection(), "file_new_vcd");
  actionFileNewMovix = new KAction(i18n("New &eMovix Project"),"video", 0, this, SLOT(slotNewMovixDoc()),
				   actionCollection(), "file_new_movix");


  actionFileNewMenu->insert( actionFileNewAudio );
  actionFileNewMenu->insert( actionFileNewData );
  actionFileNewMenu->insert( actionFileNewMixed );
  actionFileNewMenu->insert( actionFileNewVcd );
  actionFileNewMenu->insert( actionFileNewMovix );
  actionFileNewMenu->setDelayed( false );

  actionProjectAddFiles = new KAction( i18n("&Add Files..."), "filenew", 0, this, SLOT(slotProjectAddFiles()),
				       actionCollection(), "project_add_files");

  actionViewDirView = new KToggleAction(i18n("Show Directories"), 0, this, SLOT(slotShowDirView()),
				  actionCollection(), "view_dir");

  actionViewAudioPlayer = new KToggleAction(i18n("Show Audio Player"), 0, this, SLOT(slotViewAudioPlayer()),
					    actionCollection(), "view_audio_player");

  actionViewDocumentHeader = new KToggleAction(i18n("Show Document Header"), 0, this, SLOT(slotViewDocumentHeader()),
					       actionCollection(), "view_document_header");

  actionToolsBlankCdrw = new KAction(i18n("&Erase CD-RW..."), "cdrwblank", 0, this, SLOT(slotBlankCdrw()),
			       actionCollection(), "tools_blank_cdrw" );
  actionToolsDivxEncoding = new KAction(i18n("&Encode Video..."),"gear", 0, this, SLOT( slotDivxEncoding() ),
			    actionCollection(), "tools_encode_video");
  actionToolsWriteIsoImage = new KAction(i18n("&Write ISO Image..."), "gear", 0, this, SLOT(slotWriteIsoImage()),
					 actionCollection(), "tools_write_iso" );

  actionToolsWriteBinImage = new KAction(i18n("&Write Bin/Cue Image..."), "gear", 0, this, SLOT(slotWriteBinImage()),
					 actionCollection(), "tools_write_bin" );

  actionCdCopy = new KAction(i18n("&Copy CD..."), "cdcopy", 0, this, SLOT(slotCdCopy()),
			     actionCollection(), "tools_copy_cd" );

  actionSettingsK3bSetup = new KAction(i18n("K3b &Setup"), "configure", 0, this, SLOT(slotK3bSetup()),
				       actionCollection(), "settings_k3bsetup" );


  // Project actions
  // ==============================================================================================================

  // Data Project
  actionDataImportSession = new KAction(i18n("&Import Session"), "gear", 0, this, SLOT(slotDataImportSession()),
					actionCollection(), "project_data_import_session" );
  actionDataClearImportedSession = new KAction(i18n("&Clear imported Session"), "gear", 0, this,
					       SLOT(slotDataClearImportedSession()), actionCollection(),
					       "project_data_clear_imported_session" );
   actionDataEditBootImages = new KAction(i18n("&Edit boot images"), "cdtrack", 0, this,
 					 SLOT(slotEditBootImages()), actionCollection(),
 					 "project_data_edit_boot_images" );

  // ==============================================================================================================


  actionFileNewMenu->setStatusText(i18n("Creates a new project"));
  actionFileNewData->setStatusText( i18n("Creates a new data project") );
  actionFileNewAudio->setStatusText( i18n("Creates a new audio project") );
  actionToolsBlankCdrw->setStatusText( i18n("Opens CD-blanking dialog") );
  actionFileOpen->setStatusText(i18n("Opens an existing project"));
  actionFileOpenRecent->setStatusText(i18n("Opens a recently used file"));
  actionFileSave->setStatusText(i18n("Saves the actual project"));
  actionFileSaveAs->setStatusText(i18n("Saves the actual project as..."));
  actionFileClose->setStatusText(i18n("Closes the actual project"));
  actionFileQuit->setStatusText(i18n("Quits the application"));

  actionViewToolBar->setStatusText(i18n("Enables/disables the toolbar"));
  actionViewStatusBar->setStatusText(i18n("Enables/disables the statusbar"));


  createGUI();
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
  mainDock = createDockWidget( "Workspace", SmallIcon("idea") );
  setView( mainDock );
  setMainDockWidget( mainDock );
  mainDock->setDockSite( KDockWidget::DockCorner );
  mainDock->setEnableDocking( KDockWidget::DockNone );


  // --- Document Dock ----------------------------------------------------------------------------
  QWidget* documentHull = new QWidget( mainDock );
  QGridLayout* documentHullLayout = new QGridLayout( documentHull );
  documentHullLayout->setMargin( 0 );
  documentHullLayout->setSpacing( 0 );

  m_documentHeader = new QWidget( documentHull );
  QGridLayout* documentHeaderLayout = new QGridLayout( m_documentHeader );
  documentHeaderLayout->setMargin( 0 );
  documentHeaderLayout->setSpacing( 0 );

  QLabel* leftDocPicLabel = new QLabel( m_documentHeader );
  QLabel* centerDocLabel = new QLabel( m_documentHeader );
  QLabel* rightDocPicLabel = new QLabel( m_documentHeader );

  leftDocPicLabel->setPixmap( QPixmap(locate( "data", "k3b/pics/k3bprojectview_left.png" )) );
  rightDocPicLabel->setPixmap( QPixmap(locate( "data", "k3b/pics/k3bprojectview_right.png" )) );
  centerDocLabel->setText( i18n("Current Projects") );
  centerDocLabel->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  centerDocLabel->setPaletteBackgroundColor( QColor(201, 208, 255) );
  centerDocLabel->setPaletteForegroundColor( Qt::white );
  QFont f(centerDocLabel->font());
  f.setBold(true);
  centerDocLabel->setFont(f);

  documentHeaderLayout->addWidget( leftDocPicLabel, 0, 0 );
  documentHeaderLayout->addWidget( centerDocLabel, 0, 1 );
  documentHeaderLayout->addWidget( rightDocPicLabel, 0, 2 );
  documentHeaderLayout->setColStretch( 1, 1 );

  // add the document tab to the styled document box
  m_documentTab = new K3bProjectTabWidget( documentHull );
  //  m_documentTab->setPalette( oldPal );

  documentHullLayout->addWidget( m_documentHeader, 0, 0 );
  documentHullLayout->addWidget( m_documentTab, 1, 0 );

  mainDock->setWidget( documentHull );
  connect( m_documentTab, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotCurrentDocChanged(QWidget*)) );

  // fill the tabs action menu
  m_documentTab->insertAction( actionFileSave );
  m_documentTab->insertAction( actionFileSaveAs );
  m_documentTab->insertAction( actionFileClose );
  m_documentTab->insertAction( actionFileBurn );
  // ---------------------------------------------------------------------------------------------


  // --- Directory Dock --------------------------------------------------------------------------
  dirDock = createDockWidget( "K3b Dir View", SmallIcon("idea") );
  m_dirView = new K3bDirView( dirDock );
  dirDock->setWidget( m_dirView );
  dirDock->setEnableDocking( KDockWidget::DockCorner );
  dirDock->manualDock( mainDock, KDockWidget::DockTop, 30 );

  connect( dirDock, SIGNAL(iMBeingClosed()), this, SLOT(slotDirDockHidden()) );
  connect( dirDock, SIGNAL(hasUndocked()), this, SLOT(slotDirDockHidden()) );
  // ---------------------------------------------------------------------------------------------


  // --- Audioplayer Dock ------------------------------------------------------------------------
  m_audioPlayerDock = createDockWidget( "K3b Audio Player", SmallIcon("1rightarrow") );
  m_audioPlayer = new K3bAudioPlayer( this, "k3b_audio_player" );
  m_audioPlayerDock->setWidget( m_audioPlayer );
  m_audioPlayerDock->setEnableDocking( KDockWidget::DockCorner );
  m_audioPlayerDock->manualDock( mainDock, KDockWidget::DockBottom, m_audioPlayer->height() );

  connect( m_audioPlayerDock, SIGNAL(iMBeingClosed()), this, SLOT(slotAudioPlayerHidden()) );
  connect( m_audioPlayerDock, SIGNAL(hasUndocked()), this, SLOT(slotAudioPlayerHidden()) );
  // ---------------------------------------------------------------------------------------------
}


void K3bMainWindow::createClient(K3bDoc* doc)
{
  K3bView* w = doc->newView( m_documentTab );
  w->installEventFilter(this);
  doc->addView(w);
  w->setIcon(kapp->miniIcon());
  m_documentTab->insertTab( w, w->caption(), 0 );
  m_documentTab->showPage( w );

  actionFileBurn->setEnabled( true );
  actionFileSave->setEnabled( true );
  actionFileSaveAs->setEnabled( true );
  actionProjectAddFiles->setEnabled( true );

  slotCurrentDocChanged( m_documentTab->currentPage() );
}


K3bView* K3bMainWindow::activeView() const
{
  if( !pDocList->isEmpty() ) {
    QWidget* w = m_documentTab->currentPage();
    if( K3bView* view = dynamic_cast<K3bView*>(w) )
      return view;
    else
      return 0;
  }
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


void K3bMainWindow::openDocumentFile(const KURL& url)
{
  slotStatusMsg(i18n("Opening file..."));
  K3bDoc* doc;
  // check, if document already open. If yes, set the focus to the first view
  for(doc=pDocList->first(); doc > 0; doc=pDocList->next())
    {
      if(doc->URL()==url)
	{
	  K3bView* view=doc->firstView();
	  view->setFocus();
	  return;
	}
    }

  doc = K3bDoc::openDocument( url );

  if( doc == 0 )
    {
      KMessageBox::error (this,i18n("Could not open document!"), i18n("Error!"));
      return;
    }

  actionFileOpenRecent->addURL(url);

  pDocList->append(doc);

  // create the window
  createClient(doc);
}


void K3bMainWindow::saveOptions()
{
  m_config->setGroup("General Options");
  saveMainWindowSettings( m_config );
  m_config->writeEntry("Show Document Header", m_documentHeader->isVisible());
  actionFileOpenRecent->saveEntries(m_config,"Recent Files");

  // save dock positions!
  manager()->writeConfig( m_config, "Docking Config" );

  m_config->setGroup( "External Programs" );
  K3bExternalBinManager::self()->saveConfig( m_config );
  K3bDeviceManager::self()->saveConfig( m_config );

  m_dirView->saveConfig( config() );
}


void K3bMainWindow::readOptions()
{
  m_config->setGroup("General Options");

  // toolbar and statusbar settings
  applyMainWindowSettings( m_config );
  actionViewToolBar->setChecked( !toolBar()->isHidden() );
  actionViewStatusBar->setChecked( !statusBar()->isHidden() );

  bool bViewDocumentHeader = m_config->readBoolEntry("Show Document Header", true);
  actionViewDocumentHeader->setChecked(bViewDocumentHeader);
  slotViewDocumentHeader();

  // initialize the recent file list
  actionFileOpenRecent->loadEntries(m_config,"Recent Files");

  // read dock-positions
  manager()->readConfig( m_config, "Docking Config" );

  m_config->setGroup("ISO Options");
  m_useID3TagForMp3Renaming = m_config->readBoolEntry("Use ID3 Tag for mp3 renaming", false);
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

  while( K3bView* view = activeView() )
  {
    if( !view->close(true) )
      return false;
  }

  return true;
}


bool K3bMainWindow::eventFilter(QObject* object, QEvent* event)
{
  if( (event->type() == QEvent::Close) && ((K3bMainWindow*)object != this) )
    {
      QCloseEvent* e=(QCloseEvent*)event;

      K3bView* pView = (K3bView*)object;
      if( pView ) {
	K3bDoc* pDoc = pView->getDocument();

	// ---------------------------------
	// if it is not the last view we can remove it
	// if it is the last view we need to ask the user (that is done in canCloseDocument())
	// since K3bView has set the WDestructiveClose flag there is no need wondering about
	// the proper garbage collection with the views
	// ---------------------------------
	if( ( pDoc->isLastView() && canCloseDocument(pDoc) )  ||
	    !pDoc->isLastView() ) {

	  pDoc->removeView( pView );
	  m_documentTab->removePage( pView );

	  slotCurrentDocChanged( m_documentTab->currentPage() );

	  // ---------------------------------
	  // if it was the last view we removed the doc should be removed
	  // ---------------------------------
	  if( !pDoc->firstView() ) {
	    pDocList->remove( pDoc );
	  }
	  e->accept();
	  return false;
	}
	else {
	  e->ignore();
	  return true;
	}

      }
    }

  return QWidget::eventFilter( object, event );    // standard event processing
}


bool K3bMainWindow::canCloseDocument( K3bDoc* doc )
{
  if( !doc->isModified() ) {
    return true;
  }

  switch ( KMessageBox::warningYesNoCancel(this, i18n("%1 has unsaved data.").arg( doc->URL().fileName() ),
					   i18n("Closing project..."), i18n("&Save"), i18n("&Discard") ) )
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


void K3bMainWindow::slotFileNew()
{
  slotStatusMsg(i18n("Creating new document..."));

  openDocumentFile();
}

void K3bMainWindow::slotFileOpen()
{
  slotStatusMsg(i18n("Opening file..."));

  KURL url=KFileDialog::getOpenURL(QString::null,
				   i18n("*.k3b|K3b Projects"), this, i18n("Open File"));
  if(!url.isEmpty())
    {
      openDocumentFile(url);
      actionFileOpenRecent->addURL( url );
    }
}

void K3bMainWindow::slotFileOpenRecent(const KURL& url)
{
  slotStatusMsg(i18n("Opening file..."));

  openDocumentFile(url);
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
    else
      if( !doc->saveDocument(doc->URL()) )
	KMessageBox::error (this,i18n("Could not save the current document!"), i18n("I/O Error"));
    doc->setSaved(true);
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


    if(!url.isEmpty())
      {

	// default to ending ".k3b"
	if( url.mid( url.findRev('.')+1 ) != "k3b" ) {
	  if( url[ url.length()-1 ] != '.' )
	    url += ".";
	  url += "k3b";
	}

	if( !QFile::exists(url) ||
	    ( QFile::exists(url) &&
	      KMessageBox::questionYesNo( this, i18n("Do you want to overwrite %1").arg(url), i18n("File exists...") )
	      == KMessageBox::Yes ) ) {

	  if(!doc->saveDocument(url))
	    {
	      KMessageBox::error (this,i18n("Could not save the current document!"), i18n("I/O Error"));
	      return;
	    }
	  doc->changedViewList();
	  K3bView* view = doc->firstView();
	  m_documentTab->changeTab( view, view->caption() );  // CAUTION: Does not work for multible views!

	  actionFileOpenRecent->addURL(url);
	}
      }
  }
}


void K3bMainWindow::slotFileClose()
{
  slotStatusMsg(i18n("Closing file..."));

  if( K3bView* view = activeView() )
    {
      view->close(true);
    }

  if( pDocList->isEmpty() ) {
    actionFileSave->setEnabled(false);
    actionFileSaveAs->setEnabled(false);
    actionFileBurn->setEnabled( false );
    actionProjectAddFiles->setEnabled( false );
  }
}


void K3bMainWindow::slotFileQuit()
{
  close();
}


void K3bMainWindow::slotViewToolBar()
{
  // turn Toolbar on or off
  if(actionViewToolBar->isChecked()) {
    toolBar("mainToolBar")->show();
  }
  else {
    toolBar("mainToolBar")->hide();
  }
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

  // emit a changed signal everytime since we do not know if the user selected
  // "apply" and "cancel" or "ok"
  emit configChanged( m_config );
}


void K3bMainWindow::showOptionDialog( int index )
{
  K3bOptionDialog d( this, "SettingsDialog", true );

  d.showPage( index );

  d.exec();
}


void K3bMainWindow::slotNewAudioDoc()
{
  slotStatusMsg(i18n("Creating new Audio Project."));

  K3bAudioDoc* doc = new K3bAudioDoc( this );
  pDocList->append(doc);
  doc->newDocument();

  m_audioUntitledCount++;
  QString fileName = i18n("Audio%1").arg(m_audioUntitledCount);
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);
}

void K3bMainWindow::slotNewDataDoc()
{
  slotStatusMsg(i18n("Creating new Data Project."));

  K3bDataDoc* doc = new K3bDataDoc( this );
  pDocList->append(doc);
  doc->newDocument();

  m_dataUntitledCount++;
  QString fileName = i18n("Data%1").arg(m_dataUntitledCount);
  doc->isoOptions().setVolumeID( i18n("Data%1").arg(m_dataUntitledCount) );
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);
}


void K3bMainWindow::slotNewMixedDoc()
{
  slotStatusMsg(i18n("Creating new Mixed Mode Project."));

  K3bMixedDoc* doc = new K3bMixedDoc( this );
  pDocList->append(doc);
  doc->newDocument();

  m_mixedUntitledCount++;
  QString fileName=i18n("Mixed%1").arg(m_mixedUntitledCount);
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);
}

void K3bMainWindow::slotNewVcdDoc()
{
  slotStatusMsg(i18n("Creating new Video Project."));

  K3bVcdDoc* doc = new K3bVcdDoc( this );
  pDocList->append(doc);
  doc->newDocument();

  m_vcdUntitledCount++;
  QString fileName=i18n("Video%1").arg(m_vcdUntitledCount);
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);
}


void K3bMainWindow::slotNewMovixDoc()
{
  slotStatusMsg(i18n("Creating new eMovix Project."));

  K3bMovixDoc* doc = new K3bMovixDoc( this );
  pDocList->append(doc);
  doc->newDocument();

  m_movixUntitledCount++;
  QString fileName=i18n("eMovix%1").arg(m_movixUntitledCount);
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);
}

void K3bMainWindow::slotDivxEncoding(){
    slotStatusMsg(i18n("Creating new video encoding project."));
    K3bDivxView d( this, "divx");
    d.exec();
}

void K3bMainWindow::slotFileBurn()
{
  K3bView* view = activeView();

  if( view ) {
    K3bDoc* doc = view->getDocument();

    if( doc ) {
      // test if there is something to burn
      if( doc->numOfTracks() == 0 || doc->size() == 0 ) {
	KMessageBox::information( kapp->mainWidget(), i18n("Please add files to your project first!"),
				  i18n("No data to burn"), QString::null, false );
      }
      else {
	view->burnDialog();
      }
    }
  }
}


void K3bMainWindow::init()
{
  emit initializationInfo( i18n("Reading Options...") );


  KConfig globalConfig( K3b::globalConfig() );

  readOptions();

  // external bin manager
  // ===============================================================================
  emit initializationInfo( i18n("Searching for external programs...") );

  ::addDefaultPrograms( K3bExternalBinManager::self() );
  K3bExternalBinManager::self()->search();

  if( globalConfig.hasGroup("External Programs") ) {
    globalConfig.setGroup( "External Programs" );
    K3bExternalBinManager::self()->readConfig( &globalConfig );
  }

  if( config()->hasGroup("External Programs") ) {
    config()->setGroup( "External Programs" );
    K3bExternalBinManager::self()->readConfig( config() );
  }

  // ===============================================================================


  // device manager
  // ===============================================================================
  emit initializationInfo( i18n("Scanning for CD devices...") );

  if( !K3bDeviceManager::self()->scanbus() )
    kdDebug() << "No Devices found!" << endl;

  if( globalConfig.hasGroup("Devices") ) {
    globalConfig.setGroup( "Devices" );
    K3bDeviceManager::self()->readConfig( &globalConfig );
  }

  if( config()->hasGroup("Devices") ) {
    config()->setGroup( "Devices" );
    K3bDeviceManager::self()->readConfig( config() );
  }

  K3bDeviceManager::self()->printDevices();
  // ===============================================================================

  emit initializationInfo( i18n("Initializing CD view...") );

  m_dirView->setupFinalize( K3bDeviceManager::self() );

  // ===============================================================================
  emit initializationInfo( i18n("Reading local CDDB database...") );
  config()->setGroup("Cddb");
  QString filename = config()->readEntry("songlistPath", locateLocal("data", "k3b") + "/songlist.xml");
  m_songManager = new K3bSongManager();
  m_songManager->load( filename );

  emit initializationInfo( i18n("Ready") );
}


void K3bMainWindow::slotCurrentDocChanged( QWidget* )
{
  // check the doctype
  K3bView* view = activeView();
  if( view ) {
    switch( view->getDocument()->docType() ) {
    case K3bDoc::DATA:
      actionDataClearImportedSession->setEnabled(true);
      actionDataImportSession->setEnabled(true);
      //      actionDataEditBootImages->setEnabled(true);
      toolBar("dataToolBar")->show();
      break;
    default:
      actionDataClearImportedSession->setEnabled(false);
      actionDataImportSession->setEnabled(false);
      //      actionDataEditBootImages->setEnabled(false);
      toolBar("dataToolBar")->hide();
    }
  }
}


void K3bMainWindow::slotEditToolbars()
{
  saveMainWindowSettings( m_config, "General Options" );
  KEditToolbar dlg( actionCollection() );
  connect(&dlg, SIGNAL(newToolbarConfig()), SLOT(slotNewToolBarConfig()));
  dlg.exec();
}


void K3bMainWindow::slotNewToolBarConfig()
{
  createGUI();
  applyMainWindowSettings(m_config, "General Options");
}

QString K3bMainWindow::findTempFile( const QString& ending, const QString& d )
{
  KURL url;
  if( d.isEmpty() ) {
    config()->setGroup( "General Options" );
    url.setPath( config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) ) );
  }
  else {
    url.setPath(d + (d[d.length()-1] != '/' ? "/" : "") );
  }

  // find a free filename
  int num = 1;
  while( QFile::exists( url.path(1) + "k3b-" + QString::number( num ) + "." + ending ) )
    num++;

  return url.path(1) + "k3b-" + QString::number( num ) + "." + ending;
}


bool K3bMainWindow::eject()
{
  config()->setGroup( "General Options" );
  return !config()->readBoolEntry( "No cd eject", false );
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


void K3bMainWindow::slotWriteIsoImage()
{
  K3bIsoImageWritingDialog d( this, "isodialog" );
  d.exec();
}


void K3bMainWindow::slotWriteIsoImage( const KURL& url )
{
  K3bIsoImageWritingDialog d( this, "isodialog" );
  d.setImage( url );
  d.exec();
}

void K3bMainWindow::slotWriteBinImage()
{
  K3bBinImageWritingDialog d( this, "bindialog" );
  d.exec();
}


void K3bMainWindow::slotWriteBinImage( const KURL& url )
{
  K3bBinImageWritingDialog d( this, "bindialog" );
  d.setTocFile( url );
  d.exec();
}


void K3bMainWindow::slotProjectAddFiles()
{
  K3bDoc* doc = activeDoc();

  if( doc ) {
    QStringList urls = KFileDialog::getOpenFileNames( ".", "*", this, i18n("Select Files to add to Project") );
    if( !urls.isEmpty() )
      doc->addUrls( urls );
  }
  else
    KMessageBox::error( this, i18n("Please create a project before adding files"), i18n("No active Project"));
}


void K3bMainWindow::slotK3bSetup()
{
  KProcess p;
  p << "kdesu" << "k3bsetup --lang " + KGlobal::locale()->language();
  if( !p.start( KProcess::DontCare ) )
    KMessageBox::error( 0, i18n("Could not find kdesu to run K3bSetup with root privileges. Please run it manually as root.") );
}


void K3bMainWindow::slotCdCopy()
{
  K3bCdCopyDialog d( this );
  d.exec();
}


void K3bMainWindow::slotViewAudioPlayer()
{
  m_audioPlayerDock->changeHideShowState();
  slotCheckDockWidgetStatus();
}


void K3bMainWindow::slotShowDirView()
{
  dirDock->changeHideShowState();
  slotCheckDockWidgetStatus();
}


void K3bMainWindow::slotAudioPlayerHidden()
{
  actionViewAudioPlayer->setChecked( false );
}


void K3bMainWindow::slotDirDockHidden()
{
  actionViewDirView->setChecked( false );
}


void K3bMainWindow::slotCheckDockWidgetStatus()
{
  actionViewAudioPlayer->setChecked( m_audioPlayerDock->isVisible() );
  actionViewDirView->setChecked( dirDock->isVisible() );
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


K3bExternalBinManager* K3bMainWindow::externalBinManager()
{
  return K3bExternalBinManager::self();
}


K3bDeviceManager* K3bMainWindow::deviceManager()
{
  return K3bDeviceManager::self();
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


#include "k3b.moc"
