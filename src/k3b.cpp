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
#include <qvbox.h>
#include <qwhatsthis.h>
#include <qtooltip.h>
#include <qtoolbutton.h>
#include <qstring.h>
#include <qsplitter.h>
#include <qevent.h>
#include <qtabwidget.h>

// include files for KDE
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <klineeditdlg.h>
#include <kstddirs.h>

// application specific includes
#include "k3b.h"
#include "k3bglobals.h"
#include "k3bview.h"
#include "k3bdirview.h"
#include "audio/k3baudiodoc.h"
#include "audio/k3baudioview.h"
#include "device/k3bdevicemanager.h"
#include "audio/k3baudiotrackdialog.h"
#include "k3bcopywidget.h"
#include "k3bripperwidget.h"
#include "option/k3boptiondialog.h"
#include "k3bburnprogressdialog.h"
#include "k3bprojectburndialog.h"
#include "audio/k3baudiojob.h"
#include "data/k3bdatadoc.h"
#include "data/k3bdataview.h"
#include "data/k3bdatajob.h"
#include "cdinfo/k3bcdinfodialog.h"



K3bApp* k3bMain()
{
  K3bApp* _app = dynamic_cast<K3bApp*>( kapp->mainWidget() );
  if( !_app ) {
    qDebug( "No K3bApp found!");
    exit(1);
  }
  return _app;
}



K3bApp::K3bApp()
  : KDockMainWindow(0,"K3b")
{
  m_config=kapp->config();
  untitledCount=0;
  pDocList = new QList<K3bDoc>();
  pDocList->setAutoDelete(true);

  ///////////////////////////////////////////////////////////////////
  // call inits to invoke all other construction parts
  initStatusBar();
  initView();
  initActions();
	
  readOptions();

  ///////////////////////////////////////////////////////////////////
  // disable actions at startup
  fileSave->setEnabled(false);
  fileSaveAs->setEnabled(false);
  fileBurn->setEnabled( false );
  fileExport->setEnabled( false );

  m_audioTrackDialog = 0;
  m_optionDialog = 0;
  m_burnProgressDialog = 0;

  // setup audio cd drives here because we need access to the device manager
  init();
  m_initialized=false;
}

K3bApp::~K3bApp()
{
  delete pDocList;
}
// here we do the final initialize which can't be done in a constructor because it needs an instance of kapp, k3bapp or similar
void K3bApp::show(){
    if( !m_initialized ){
        m_initialized = true;
        m_dirView->setupFinalize(m_deviceManager);
    }
    QWidget::show();
}

void K3bApp::initActions()
{
  fileOpen = KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
  fileOpenRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const KURL&)), actionCollection());
  fileSave = KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
  fileSaveAs = KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  fileClose = KStdAction::close(this, SLOT(slotFileClose()), actionCollection());
  fileQuit = KStdAction::quit(this, SLOT(slotFileQuit()), actionCollection());
  viewToolBar = KStdAction::showToolbar(this, SLOT(slotViewToolBar()), actionCollection());
  viewStatusBar = KStdAction::showStatusbar(this, SLOT(slotViewStatusBar()), actionCollection());
  settingsConfigure = KStdAction::preferences(this, SLOT(slotSettingsConfigure()), actionCollection() );

  fileBurn = new KAction( i18n("&Burn..."), "cdwriter_unmount", 0, this, SLOT(slotFileBurn()), 
			  actionCollection(), "file_burn");
  fileExport = new KAction( i18n("E&xport..."), "revert", 0, this, SLOT(slotFileExport()), 
			    actionCollection(), "file_export" );

  fileNewMenu = new KActionMenu( i18n("&New Project"), "filenew", actionCollection(), "file_new" );
  fileNewAudio = new KAction(i18n("New &Audio project"), "sound", 0, this, SLOT(slotNewAudioDoc()), 
			     actionCollection(), "file_new_audio");
  fileNewData = new KAction(i18n("New &Data project"),"tar", 0, this, SLOT(slotNewDataDoc()), 
			    actionCollection(), "file_new_data");
  fileNewMenu->insert( fileNewAudio );
  fileNewMenu->insert( fileNewData );
  fileNewMenu->setDelayed( false );

  viewDirView = new KToggleAction(i18n("Show Directories"), "view_sidetree", 0, this, SLOT(slotShowDirView()), 
				  actionCollection(), "view_dir");

  toolsCdInfo = new KAction(i18n("CD &Info"), "cdrom_unmount", 0, this, SLOT(slotCdInfo()), 
			    actionCollection(), "tools_cd_info" );

  fileNewMenu->setStatusText(i18n("Creates a new project"));
  fileOpen->setStatusText(i18n("Opens an existing project"));
  fileOpenRecent->setStatusText(i18n("Opens a recently used file"));
  fileSave->setStatusText(i18n("Saves the actual project"));
  fileSaveAs->setStatusText(i18n("Saves the actual project as..."));
  fileClose->setStatusText(i18n("Closes the actual project"));
  fileQuit->setStatusText(i18n("Quits the application"));

  viewToolBar->setStatusText(i18n("Enables/disables the toolbar"));
  viewStatusBar->setStatusText(i18n("Enables/disables the statusbar"));

  viewDirView->setChecked( true );

  createGUI();
}


void K3bApp::initStatusBar()
{
  ///////////////////////////////////////////////////////////////////
  // STATUSBAR
  // TODO: add your own items you need for displaying current application status.
  statusBar()->insertItem(i18n("Ready."),1);
}


void K3bApp::initView()
{
  // setup main docking things
  mainDock = createDockWidget( "Workspace", SmallIcon("idea") );
  setView( mainDock );
  setMainDockWidget( mainDock );
  mainDock->setEnableDocking( KDockWidget::DockNone );

  m_documentTab = new QTabWidget( mainDock );
  mainDock->setWidget( m_documentTab );
  connect( m_documentTab, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotCurrentDocChanged(QWidget*)) );

  // add the cd-copy-widget to the tab
  //   m_documentTab->addTab( new K3bCopyWidget( m_documentTab ), "&Copy CD" );
  //  m_documentTab->addTab( new K3bRipperWidget( m_documentTab ), "&Ripping" );

  dirDock = createDockWidget( "DirDock", SmallIcon("idea") );
  m_dirView = new K3bDirView( dirDock );
  dirDock->setWidget( m_dirView );
  dirDock->setEnableDocking( KDockWidget::DockCorner );
  dirDock->manualDock( mainDock, KDockWidget::DockLeft, 30 );

  connect( dirDock, SIGNAL(headerCloseButtonClicked()), this, SLOT(slotDirDockHidden()) );
}


void K3bApp::createClient(K3bDoc* doc)
{
  K3bView* w = doc->newView( m_documentTab );
  w->installEventFilter(this);
  doc->addView(w);
  w->setIcon(kapp->miniIcon());
  m_documentTab->insertTab( w, w->caption(), 0 );
  m_documentTab->showPage( w );

  fileBurn->setEnabled( true );
  fileExport->setEnabled( true );
  fileSave->setEnabled( true );
  fileSaveAs->setEnabled( true );
}

void K3bApp::openDocumentFile(const KURL& url)
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
      KMessageBox::error (this,i18n("Could not open document !"), i18n("Error !"));
      return;	
    }

  fileOpenRecent->addURL(url);

  // create the window
  createClient(doc);

  slotStatusMsg(i18n("Ready."));
}


void K3bApp::saveOptions()
{	
  m_config->setGroup("General Options");
  m_config->writeEntry("Geometry", size());
  m_config->writeEntry("Show Toolbar", toolBar()->isVisible());
  m_config->writeEntry("Show Statusbar",statusBar()->isVisible());
  m_config->writeEntry("Show DirView",m_dirView->isVisible());
  m_config->writeEntry("ToolBarPos", (int) toolBar("mainToolBar")->barPos());
  fileOpenRecent->saveEntries(m_config,"Recent Files");

  m_config->setGroup("ISO Options");
  m_config->writeEntry( "Use ID3 Tag for mp3 renaming", m_useID3TagForMp3Renaming );

  // save dock positions!
  manager()->writeConfig( m_config, "Docking Config" );
}


void K3bApp::readOptions()
{
  m_config->setGroup("General Options");

  // bar status settings
  bool bViewToolbar = m_config->readBoolEntry("Show Toolbar", true);
  viewToolBar->setChecked(bViewToolbar);
  slotViewToolBar();

  bool bViewStatusbar = m_config->readBoolEntry("Show Statusbar", true);
  viewStatusBar->setChecked(bViewStatusbar);
  slotViewStatusBar();

  // bar position settings
  KToolBar::BarPosition toolBarPos;
  toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("ToolBarPos", KToolBar::Top);
  toolBar("mainToolBar")->setBarPos(toolBarPos);

  // initialize the recent file list
  fileOpenRecent->loadEntries(m_config,"Recent Files");

  QSize size=m_config->readSizeEntry("Geometry");
  if(!size.isEmpty())
    {
      resize(size);
    }

  // read dock-positions
  manager()->readConfig( m_config, "Docking Config" );

  m_config->setGroup("ISO Options");
  m_useID3TagForMp3Renaming = m_config->readBoolEntry("Use ID3 Tag for mp3 renaming", false);
}

void K3bApp::saveProperties(KConfig *)
{

}


void K3bApp::readProperties(KConfig*)
{
}

bool K3bApp::queryClose()
{
  QStringList saveFiles;
  //  K3bDoc* doc;
  if(pDocList->isEmpty())
    return true;

  // nothing to save so far
  return true;

//   for(doc=pDocList->first(); doc!=0;doc=pDocList->next())
//     {
//       if(doc->isModified())
// 	saveFiles.append(doc->URL().fileName());
//     }
//   if(saveFiles.isEmpty())
//     return true;

//   switch (KMessageBox::questionYesNoList(this,
// 					 i18n("One or more documents have been modified.\nSave changes before exiting?"),saveFiles))
//     {
//     case KMessageBox::Yes:
//       for(doc=pDocList->first(); doc!=0;doc=pDocList->next())
// 	{
// 	  if(doc->URL().fileName().contains(i18n("Untitled")))
// 	    slotFileSaveAs();
// 	  else
// 	    {
// 	      if(!doc->saveDocument(doc->URL()))
// 		{
// 		  KMessageBox::error (this,i18n("Could not save the current document !"), i18n("I/O Error !"));
// 		  return false;
// 		}
// 	    }
// 	}
//       return true;
//     case KMessageBox::No:
//     default:
//       return true;
//     }
}

bool K3bApp::queryExit()
{
  saveOptions();
  return true;
}

bool K3bApp::eventFilter(QObject* object, QEvent* event)
{
  if((event->type() == QEvent::Close)&&((K3bApp*)object!=this))
    {
      QCloseEvent* e=(QCloseEvent*)event;

      K3bView* pView=(K3bView*)object;
      if( pView ) {
	K3bDoc* pDoc=pView->getDocument();
	if(pDoc->canCloseFrame(pView))
	  {
	    pDoc->removeView(pView);
	    m_documentTab->removePage( pView );
	    if(!pDoc->firstView())
	      pDocList->remove(pDoc);
	    e->accept();
	  }
	else
	  e->ignore();
      }
    }
  return QWidget::eventFilter( object, event );    // standard event processing
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////


void K3bApp::slotFileNew()
{
  slotStatusMsg(i18n("Creating new document..."));

  openDocumentFile();

  slotStatusMsg(i18n("Ready."));
}

void K3bApp::slotFileOpen()
{
  slotStatusMsg(i18n("Opening file..."));
	
  KURL url=KFileDialog::getOpenURL(QString::null,
				   i18n("*.k3b|K3b Projects"), this, i18n("Open File..."));
  if(!url.isEmpty())
    {
      openDocumentFile(url);
      fileOpenRecent->addURL( url );
    }

  slotStatusMsg(i18n("Ready."));
}

void K3bApp::slotFileOpenRecent(const KURL& url)
{
  slotStatusMsg(i18n("Opening file..."));
  	
  openDocumentFile(url);
	
  slotStatusMsg(i18n("Ready."));
}

void K3bApp::slotFileSave()
{
  slotStatusMsg(i18n("Saving file..."));

  K3bView* m = dynamic_cast<K3bView*>(m_documentTab->currentPage() );
  if( m )
    {
      K3bDoc* doc = m->getDocument();
      if(doc->URL().fileName().contains(i18n("Untitled")))
	slotFileSaveAs();
      else
	if(!doc->saveDocument(doc->URL()))
	  KMessageBox::error (this,i18n("Could not save the current document !"), i18n("I/O Error !"));
    }

  slotStatusMsg(i18n("Ready."));
}

void K3bApp::slotFileSaveAs()
{
  slotStatusMsg(i18n("Saving file with a new filename..."));

  KURL url=KFileDialog::getSaveURL(QDir::currentDirPath(),
				   i18n("*.k3b|K3b Projects"), this, i18n("Save as..."));
  if(!url.isEmpty())
    {
      // TODO: if the file exists, ask for owerwrite

      K3bView* m = dynamic_cast<K3bView*>(m_documentTab->currentPage() );
      if( m )
	{
	  K3bDoc* doc =	m->getDocument();
	  if(!doc->saveDocument(url))
	    {
	      KMessageBox::error (this,i18n("Could not save the current document !"), i18n("I/O Error !"));
	      return;
	    }
	  doc->changedViewList();
	  m_documentTab->changeTab( m, m->caption() );   // does not fit with the multible view architecture !!!
	  //setWndTitle(m);
	  fileOpenRecent->addURL(url);
	}	
    }

  slotStatusMsg(i18n("Ready."));
}


void K3bApp::slotFileExport()
{
  if( K3bAudioView* m = dynamic_cast<K3bAudioView*>( m_documentTab->currentPage() ) ) {
    QString file = KFileDialog::getSaveFileName( QDir::home().absPath(), "*.toc", k3bMain(), i18n("Export to cdrdao-toc-file") );
    if( !file.isEmpty() ) {
      if( !((K3bAudioDoc*)m->getDocument())->writeTOC( file ) )
	KMessageBox::error( this, i18n("Could not write to file %1").arg( file ), i18n("I/O Error") );
    }
  }
  else if( K3bDataView* m = dynamic_cast<K3bDataView*>( m_documentTab->currentPage() ) ) {
    QString file = KFileDialog::getSaveFileName( QDir::home().absPath(), "*.mkisofs", k3bMain(), i18n("Export to mkisofs-pathspec-file") );
    if( !file.isEmpty() ) {
      if( ((K3bDataDoc*)m->getDocument())->writePathSpec( file ).isEmpty() )
	KMessageBox::error( this, i18n("Could not write to file %1").arg( file ), i18n("I/O Error") );
    }
  }
}


void K3bApp::slotFileClose()
{
  slotStatusMsg(i18n("Closing file..."));
  K3bView* m = dynamic_cast<K3bView*>( m_documentTab->currentPage() );
  if( m )
    {
      K3bDoc* doc=m->getDocument();
      doc->closeDocument();
    }
	
  slotStatusMsg(i18n("Ready."));
}


void K3bApp::slotFileQuit()
{
  slotStatusMsg(i18n("Exiting..."));
  //  saveOptions();
  // close the first window, the list makes the next one the first again.
  // This ensures that queryClose() is called on each window to ask for closing
  //  KMainWindow* w;
  //  if(memberList)
  //  {
  //    for(w=memberList->first(); w!=0; w=memberList->first())
  //    {
  //      // only close the window if the closeEvent is accepted. If the user presses Cancel on the saveModified() dialog,
  //      // the window and the application stay open.
  //      if(!w->close())
  //        break;
  //    }
  //  }	

  close();		
}


void K3bApp::slotViewToolBar()
{
  slotStatusMsg(i18n("Toggle the toolbar..."));
  ///////////////////////////////////////////////////////////////////
  // turn Toolbar on or off
  if(!viewToolBar->isChecked())
    {
      toolBar("mainToolBar")->hide();
    }
  else
    {
      toolBar("mainToolBar")->show();
    }		

  slotStatusMsg(i18n("Ready."));
}

void K3bApp::slotViewStatusBar()
{
  slotStatusMsg(i18n("Toggle the statusbar..."));
  ///////////////////////////////////////////////////////////////////
  //turn Statusbar on or off
  if(!viewStatusBar->isChecked())
    {
      statusBar()->hide();
    }
  else
    {
      statusBar()->show();
    }

  slotStatusMsg(i18n("Ready."));
}


void K3bApp::slotStatusMsg(const QString &text)
{
  ///////////////////////////////////////////////////////////////////
  // change status message permanently
  statusBar()->clear();
  statusBar()->changeItem(text,1);
}


void K3bApp::slotShowDirView()
{
  // undock and hide or 'redock' and show
  if( !dirDock->isVisible() ) {
    dirDock->manualDock( mainDock, KDockWidget::DockLeft, 30 );
    manager()->readConfig( m_config, "Docking Config" );
    dirDock->show();
    viewDirView->setChecked( true );
  }
  else {
    dirDock->hide();
    dirDock->undock();
    viewDirView->setChecked( false );
  }
}


void K3bApp::slotSettingsConfigure()
{
  if( !m_optionDialog )
    m_optionDialog = new K3bOptionDialog( this, "SettingsDialog", true );
		
  if( !m_optionDialog->isVisible() )
    m_optionDialog->show();
}


void K3bApp::showOptionDialog( int index )
{
  if( !m_optionDialog )
    m_optionDialog = new K3bOptionDialog( this, "SettingsDialog", true );

  m_optionDialog->showPage( index );
	
  if( !m_optionDialog->isVisible() )
    m_optionDialog->show();
}


void K3bApp::searchExternalProgs()
{
  m_config->setGroup("External Programs");

  QString _cdrdao;
  QString _cdrecord;
  QString _mpg123;
  QString _sox;

  if( !m_config->hasKey("cdrdao path") ) {
    if( QFile::exists( "/usr/bin/cdrdao" ) ) {
      _cdrdao = "/usr/bin/cdrdao";
      qDebug("(K3bApp) found cdrdao in " + _cdrdao );
    }
    else if( QFile::exists( "/usr/local/bin/cdrdao" ) ) {
      _cdrdao = "/usr/local/bin/cdrdao";
      qDebug("(K3bApp) found cdrdao in " + _cdrdao );
    }
    else {
      bool ok = true;
      while( !QFile::exists( _cdrdao ) && ok )
	_cdrdao = KLineEditDlg::getText( "Could not find cdrdao. Please insert the full path...", "cdrdao", &ok, this );
    }
    m_config->writeEntry( "cdrdao path", _cdrdao );
  }

  if( !m_config->hasKey("sox path") ) {
    if( QFile::exists( "/usr/bin/sox" ) ) {
      _sox = "/usr/bin/sox";
      qDebug("(K3bApp) found sox in " + _sox );
    }
    else if( QFile::exists( "/usr/local/bin/sox" ) ) {
      _sox = "/usr/local/bin/sox";
      qDebug("(K3bApp) found sox in " + _sox );
    }
    else {
      bool ok = true;
      while( !QFile::exists( _sox ) && ok )
	_sox = KLineEditDlg::getText( "Could not find sox. Please insert the full path...", "sox", &ok, this );
    }
    m_config->writeEntry( "sox path", _sox );
  }

  if( !m_config->hasKey("cdrecord path") ) {
    if( QFile::exists( "/usr/bin/cdrecord" ) ) {
      _cdrecord = "/usr/bin/cdrecord";
      qDebug("(K3bApp) found cdrecord in " + _cdrecord );
    }
    else if( QFile::exists( "/usr/local/bin/cdrecord" ) ) {
      _cdrecord = "/usr/local/bin/cdrecord";
      qDebug("(K3bApp) found cdrecord in " + _cdrecord );
    }
    else {
      bool ok = true;
      while( !QFile::exists( _cdrecord ) && ok )
	_cdrecord = KLineEditDlg::getText( "Could not find cdrecord. Please insert the full path...", "cdrecord", &ok, this );
    }
    m_config->writeEntry( "cdrecord path", _cdrecord );
  }
	
  if( !m_config->hasKey( "mpg123 path" ) ) {
    if( QFile::exists( "/usr/bin/mpg123" ) ) {
      _mpg123 = "/usr/bin/mpg123";
      qDebug("(K3bApp) found mpg123 in " + _mpg123 );
    }
    else if( QFile::exists( "/usr/local/bin/mpg123" ) ) {
      _mpg123 = "/usr/local/bin/mpg123";
      qDebug("(K3bApp) found mpg123 in " + _mpg123 );
    }
    else {
      bool ok = true;
      while( !QFile::exists( _mpg123 ) && ok )
	_mpg123 = KLineEditDlg::getText( "Could not find mpg123. Please insert the full path...", "mpg123", &ok, this );
    }
    m_config->writeEntry( "mpg123 path", _mpg123 );
  }
	
  m_config->sync();
}

void K3bApp::slotNewAudioDoc()
{
  slotStatusMsg(i18n("Creating new Audio Project."));

  K3bAudioDoc* doc = new K3bAudioDoc( this );
  pDocList->append(doc);
  doc->newDocument();

  untitledCount+=1;
  QString fileName=QString(i18n("Untitled%1")).arg(untitledCount);
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  // create the window
  createClient(doc);

  slotStatusMsg(i18n("Ready."));
}

void K3bApp::slotNewDataDoc()
{
  slotStatusMsg(i18n("Creating new Data Project."));

  K3bDataDoc* doc = new K3bDataDoc( this );
  pDocList->append(doc);
  doc->newDocument();

  untitledCount+=1;
  QString fileName=QString(i18n("Untitled%1")).arg(untitledCount);
  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);
  doc->setVolumeID( QString("ISO_%1").arg(untitledCount) );

  // create the window
  createClient(doc);

  slotStatusMsg(i18n("Ready."));
}

K3bAudioTrackDialog* K3bApp::audioTrackDialog()
{
  if( !m_audioTrackDialog )
    m_audioTrackDialog = new K3bAudioTrackDialog( this );
		
  return m_audioTrackDialog;
}

void K3bApp::slotFileBurn()
{
  QWidget* w = m_documentTab->currentPage();
  if( w )
    {
      if( K3bView* _view = dynamic_cast<K3bView*>(w) ) {
	K3bDoc* doc = _view->getDocument();
				
	if( doc )
	  {
	    // test if there is something to burn
	    if( doc->numOfTracks() == 0 ) {
	      KMessageBox::information( kapp->mainWidget(), "There is nothing to burn!", "So what?", QString::null, false );
	      return;
	    }
				
	    if( _view->burnDialog()->exec(true) == K3bProjectBurnDialog::Burn ) {
	      if( !m_burnProgressDialog )
		m_burnProgressDialog = new K3bBurnProgressDialog( this );
					
	      K3bBurnJob* job = _view->getDocument()->newBurnJob();
				
	      m_burnProgressDialog->setJob( job );
					
	      // BAD!!!! :-(( the job is deleted before the burnprocessdialog is hidden!!!
	      connect( job, SIGNAL(finished( K3bJob* )), this, SLOT(slotJobFinished( K3bJob* )) );
	      m_burnProgressDialog->show();
	      job->start();
	    }
	  }
      }

      else if( w->inherits( "K3bCopyWidget" ) ) {
	// TODO: do whatever to copy a cd
      }
    }
}


void K3bApp::init()
{
  searchExternalProgs();

  m_deviceManager = new K3bDeviceManager( this );

  if( !m_deviceManager->scanbus() )
    qDebug( "No Devices found!" );

  if( config()->hasGroup("Devices") ) {
    config()->setGroup( "Devices" );
    m_deviceManager->readConfig( config() );
  }
			
  m_deviceManager->printDevices();
}


void K3bApp::slotDirDockHidden()
{
  // save dock positions!
  manager()->writeConfig( m_config, "Docking Config" );
  viewDirView->setChecked( false );
}


void K3bApp::slotCurrentDocChanged( QWidget* w )
{
  if( w->inherits( "K3bView" ) ) {
    // activate actions for file-handling
    fileClose->setEnabled( true );
    fileSave->setEnabled( true );
    fileSaveAs->setEnabled( true );
    fileExport->setEnabled( true );
  }
  else {
    // the active window does not represent a file (e.g. the copy-widget)
    fileClose->setEnabled( false );
    fileSave->setEnabled( false );
    fileSaveAs->setEnabled( false );
  }
}


QString K3bApp::findTempFile( const QString& ending, const QString& d )
{
  QString dir(d);
  if( dir.isEmpty() ) {
    config()->setGroup( "General Options" );
    dir = config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) );
  }
  if( dir.at(dir.length() - 1) != '/' )
    dir += "/";
	
  // find a free filename
  int num = 1;
  while( QFile::exists( dir + "k3b-" + QString::number( num ) + "." + ending ) )
    num++;

  return dir + "k3b-" + QString::number( num ) + "." + ending;
}


bool K3bApp::eject()
{
  config()->setGroup( "Writing Options" );
  return config()->readBoolEntry( "Eject when finished", true );
}


void K3bApp::slotJobFinished( K3bJob* job )
{
  job->disconnect();
  delete job;
}


void K3bApp::slotErrorMessage(const QString& message)
{
  KMessageBox::error( this, message );
}


void K3bApp::slotWarningMessage(const QString& message)
{
  KMessageBox::sorry( this, message );
}


void K3bApp::slotCdInfo()
{
  K3bCdInfoDialog* d = new K3bCdInfoDialog( this, "cdinfod", true );
  d->exec();
  delete d;
}
