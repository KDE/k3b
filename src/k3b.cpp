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
#include "option/k3boptiondialog.h"
#include "k3bburnprogressdialog.h"
#include "k3bprojectburndialog.h"
#include "audio/k3baudiojob.h"
#include "data/k3bdatadoc.h"
#include "data/k3bdataview.h"
#include "data/k3bdatajob.h"
#include "cdinfo/k3bcdinfodialog.h"
#include "k3bblankingdialog.h"


K3bMainWindow* k3bMain()
{
  K3bMainWindow* _app = dynamic_cast<K3bMainWindow*>( kapp->mainWidget() );
  if( !_app ) {
    qDebug( "No K3bMainWindow found!");
    exit(1);
  }
  return _app;
}



K3bMainWindow::K3bMainWindow()
  : KDockMainWindow(0,"K3b")
{
  m_config = kapp->config();
  untitledCount = 0;
  pDocList = new QList<K3bDoc>();
  pDocList->setAutoDelete(true);

  ///////////////////////////////////////////////////////////////////
  // call inits to invoke all other construction parts
  initStatusBar();
  initView();
  initActions();

  ///////////////////////////////////////////////////////////////////
  // disable actions at startup
  actionFileSave->setEnabled(false);
  actionFileSaveAs->setEnabled(false);
  actionFileBurn->setEnabled( false );
  actionFileExport->setEnabled( false );

  m_audioTrackDialog = 0;
  m_optionDialog = 0;
  m_burnProgressDialog = 0;
}

K3bMainWindow::~K3bMainWindow()
{
  delete pDocList;
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

  actionFileBurn = new KAction( i18n("&Burn..."), "cdwriter_unmount", 0, this, SLOT(slotFileBurn()), 
			  actionCollection(), "file_burn");
  actionFileExport = new KAction( i18n("E&xport..."), "revert", 0, this, SLOT(slotFileExport()), 
			    actionCollection(), "file_export" );

  actionFileNewMenu = new KActionMenu( i18n("&New Project"), "filenew", actionCollection(), "file_new" );
  actionFileNewAudio = new KAction(i18n("New &Audio project"), "sound", 0, this, SLOT(slotNewAudioDoc()), 
			     actionCollection(), "file_new_audio");
  actionFileNewData = new KAction(i18n("New &Data project"),"tar", 0, this, SLOT(slotNewDataDoc()), 
			    actionCollection(), "file_new_data");

  actionFileNewMenu->insert( actionFileNewAudio );
  actionFileNewMenu->insert( actionFileNewData );
  actionFileNewMenu->setDelayed( false );

  actionViewDirView = new KToggleAction(i18n("Show Directories"), "view_sidetree", 0, this, SLOT(slotShowDirView()), 
				  actionCollection(), "view_dir");

  actionToolsCdInfo = new KAction(i18n("CD &Info"), "cdinfo", 0, this, SLOT(slotCdInfo()), 
			    actionCollection(), "tools_cd_info" );

  actionToolsBlankCdrw = new KAction(i18n("&Blank CD-RW"), "cdrwblank", 0, this, SLOT(slotBlankCdrw()), 
			       actionCollection(), "tools_blank_cdrw" );

  actionFileNewMenu->setStatusText(i18n("Creates a new project"));
  actionFileNewData->setStatusText( i18n("Creates a new data project") );
  actionFileNewAudio->setStatusText( i18n("Creates a new audio project") );
  actionToolsBlankCdrw->setStatusText( i18n("Opens CD-blanking dialog") );
  actionToolsCdInfo->setStatusText( i18n("Show information on a disk") );
  actionFileOpen->setStatusText(i18n("Opens an existing project"));
  actionFileOpenRecent->setStatusText(i18n("Opens a recently used file"));
  actionFileSave->setStatusText(i18n("Saves the actual project"));
  actionFileSaveAs->setStatusText(i18n("Saves the actual project as..."));
  actionFileClose->setStatusText(i18n("Closes the actual project"));
  actionFileQuit->setStatusText(i18n("Quits the application"));

  actionViewToolBar->setStatusText(i18n("Enables/disables the toolbar"));
  actionViewStatusBar->setStatusText(i18n("Enables/disables the statusbar"));

  actionViewDirView->setChecked( true );

  createGUI();
}


void K3bMainWindow::initStatusBar()
{
  ///////////////////////////////////////////////////////////////////
  // STATUSBAR
  // TODO: add your own items you need for displaying current application status.
  //  statusBar()->insertItem(i18n("Ready."),1);
}


void K3bMainWindow::initView()
{
  // setup main docking things
  mainDock = createDockWidget( "Workspace", SmallIcon("idea") );
  setView( mainDock );
  setMainDockWidget( mainDock );
  mainDock->setEnableDocking( KDockWidget::DockNone );

  m_documentTab = new QTabWidget( mainDock );
  mainDock->setWidget( m_documentTab );
  connect( m_documentTab, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotCurrentDocChanged(QWidget*)) );


  dirDock = createDockWidget( "DirDock", SmallIcon("idea") );
  m_dirView = new K3bDirView( dirDock );
  dirDock->setWidget( m_dirView );
  dirDock->setEnableDocking( KDockWidget::DockCorner );
  dirDock->manualDock( mainDock, KDockWidget::DockLeft, 30 );

  connect( dirDock, SIGNAL(headerCloseButtonClicked()), this, SLOT(slotDirDockHidden()) );
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
  actionFileExport->setEnabled( true );
  actionFileSave->setEnabled( true );
  actionFileSaveAs->setEnabled( true );
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
      KMessageBox::error (this,i18n("Could not open document !"), i18n("Error !"));
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
  m_config->writeEntry("Geometry", size());
  m_config->writeEntry("Show Toolbar", toolBar()->isVisible());
  m_config->writeEntry("Show Statusbar",statusBar()->isVisible());
  m_config->writeEntry("Show DirView",m_dirView->isVisible());
  m_config->writeEntry("ToolBarPos", (int) toolBar("mainToolBar")->barPos());
  actionFileOpenRecent->saveEntries(m_config,"Recent Files");

  m_config->setGroup("ISO Options");
  m_config->writeEntry( "Use ID3 Tag for mp3 renaming", m_useID3TagForMp3Renaming );

  // save dock positions!
  manager()->writeConfig( m_config, "Docking Config" );
}


void K3bMainWindow::readOptions()
{
  m_config->setGroup("General Options");

  // bar status settings
  bool bViewToolbar = m_config->readBoolEntry("Show Toolbar", true);
  actionViewToolBar->setChecked(bViewToolbar);
  slotViewToolBar();

  bool bViewStatusbar = m_config->readBoolEntry("Show Statusbar", true);
  actionViewStatusBar->setChecked(bViewStatusbar);
  slotViewStatusBar();

  // bar position settings
  KToolBar::BarPosition toolBarPos;
  toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("ToolBarPos", KToolBar::Top);
  toolBar("mainToolBar")->setBarPos(toolBarPos);

  // initialize the recent file list
  actionFileOpenRecent->loadEntries(m_config,"Recent Files");

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

  while( K3bView* view = (K3bView*)m_documentTab->currentPage() )
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
				   i18n("*.k3b|K3b Projects"), this, i18n("Open File..."));
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
  K3bView* m = dynamic_cast<K3bView*>(m_documentTab->currentPage() );
  if( m ) {
    K3bDoc* doc = m->getDocument();
    fileSave( doc );
  }

}

void K3bMainWindow::fileSave( K3bDoc* doc )
{
  slotStatusMsg(i18n("Saving file..."));

  if( doc == 0 ) {
    K3bView* m = dynamic_cast<K3bView*>(m_documentTab->currentPage() );
    if( m )
      doc = m->getDocument();
  }
  if( doc != 0 ) {
    if( doc->URL().fileName().contains(i18n("Untitled")) )
      fileSaveAs( doc );
    else
      if( !doc->saveDocument(doc->URL()) )
	KMessageBox::error (this,i18n("Could not save the current document !"), i18n("I/O Error !"));
  }
}


void K3bMainWindow::slotFileSaveAs()
{
  K3bView* m = dynamic_cast<K3bView*>(m_documentTab->currentPage() );
  if( m ) {
    K3bDoc* doc = m->getDocument();
    fileSaveAs( doc );
  }
}


void K3bMainWindow::fileSaveAs( K3bDoc* doc )
{
  slotStatusMsg(i18n("Saving file with a new filename..."));

  if( doc == 0 ) {
    K3bView* m = dynamic_cast<K3bView*>(m_documentTab->currentPage() );
    if( m )
      doc = m->getDocument();
  }

  if( doc != 0 ) {

    QString url = KFileDialog::getSaveFileName(QDir::currentDirPath(),
					       i18n("*.k3b|K3b Projects"), this, i18n("Save as..."));
    
    
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
	      KMessageBox::error (this,i18n("Could not save the current document !"), i18n("I/O Error !"));
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


void K3bMainWindow::slotFileExport()
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


void K3bMainWindow::slotFileClose()
{
  slotStatusMsg(i18n("Closing file..."));

  K3bView* m = dynamic_cast<K3bView*>( m_documentTab->currentPage() );
  if( m )
    {
      m->close(true);
    }
}


void K3bMainWindow::slotFileQuit()
{
  close();		
}


void K3bMainWindow::slotViewToolBar()
{
  // turn Toolbar on or off
  if(!actionViewToolBar->isChecked())
    {
      toolBar("mainToolBar")->hide();
    }
  else
    {
      toolBar("mainToolBar")->show();
    }		
}

void K3bMainWindow::slotViewStatusBar()
{
  //turn Statusbar on or off
  if(!actionViewStatusBar->isChecked())
    {
      statusBar()->hide();
    }
  else
    {
      statusBar()->show();
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


void K3bMainWindow::slotShowDirView()
{
  // undock and hide or 'redock' and show
  if( !dirDock->isVisible() ) {
    dirDock->manualDock( mainDock, KDockWidget::DockLeft, 30 );
    manager()->readConfig( m_config, "Docking Config" );
    dirDock->show();
    actionViewDirView->setChecked( true );
  }
  else {
    dirDock->hide();
    dirDock->undock();
    actionViewDirView->setChecked( false );
  }
}


void K3bMainWindow::slotSettingsConfigure()
{
  if( !m_optionDialog )
    m_optionDialog = new K3bOptionDialog( this, "SettingsDialog", true );
		
  if( !m_optionDialog->isVisible() )
    m_optionDialog->show();
}


void K3bMainWindow::showOptionDialog( int index )
{
  if( !m_optionDialog )
    m_optionDialog = new K3bOptionDialog( this, "SettingsDialog", true );

  m_optionDialog->showPage( index );
	
  if( !m_optionDialog->isVisible() )
    m_optionDialog->show();
}


void K3bMainWindow::searchExternalProgs()
{
  m_config->setGroup("External Programs");

  QString _cdrdao;
  QString _cdrecord;
  QString _mpg123;
  QString _sox;

  if( !m_config->hasKey("cdrdao path") ) {
    if( QFile::exists( "/usr/bin/cdrdao" ) ) {
      _cdrdao = "/usr/bin/cdrdao";
      qDebug("(K3bMainWindow) found cdrdao in " + _cdrdao );
    }
    else if( QFile::exists( "/usr/local/bin/cdrdao" ) ) {
      _cdrdao = "/usr/local/bin/cdrdao";
      qDebug("(K3bMainWindow) found cdrdao in " + _cdrdao );
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
      qDebug("(K3bMainWindow) found sox in " + _sox );
    }
    else if( QFile::exists( "/usr/local/bin/sox" ) ) {
      _sox = "/usr/local/bin/sox";
      qDebug("(K3bMainWindow) found sox in " + _sox );
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
      qDebug("(K3bMainWindow) found cdrecord in " + _cdrecord );
    }
    else if( QFile::exists( "/usr/local/bin/cdrecord" ) ) {
      _cdrecord = "/usr/local/bin/cdrecord";
      qDebug("(K3bMainWindow) found cdrecord in " + _cdrecord );
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
      qDebug("(K3bMainWindow) found mpg123 in " + _mpg123 );
    }
    else if( QFile::exists( "/usr/local/bin/mpg123" ) ) {
      _mpg123 = "/usr/local/bin/mpg123";
      qDebug("(K3bMainWindow) found mpg123 in " + _mpg123 );
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

void K3bMainWindow::slotNewAudioDoc()
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
}

void K3bMainWindow::slotNewDataDoc()
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
}

K3bAudioTrackDialog* K3bMainWindow::audioTrackDialog()
{
  if( !m_audioTrackDialog )
    m_audioTrackDialog = new K3bAudioTrackDialog( this );
		
  return m_audioTrackDialog;
}

void K3bMainWindow::slotFileBurn()
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
    }
}


void K3bMainWindow::init()
{
  emit initializationInfo( i18n("Reading Options...") );

  readOptions();

  emit initializationInfo( i18n("Searching for external programs...") );

  searchExternalProgs();

  emit initializationInfo( i18n("Scanning for cd devices...") );

  m_deviceManager = new K3bDeviceManager( this );

  if( !m_deviceManager->scanbus() )
    qDebug( "No Devices found!" );

  if( config()->hasGroup("Devices") ) {
    config()->setGroup( "Devices" );
    m_deviceManager->readConfig( config() );
  }
			
  m_deviceManager->printDevices();

  emit initializationInfo( i18n("Initializing cd view...") );

  m_dirView->setupFinalize( m_deviceManager );

  emit initializationInfo( i18n("Ready") );
}


void K3bMainWindow::slotDirDockHidden()
{
  // save dock positions!
  manager()->writeConfig( m_config, "Docking Config" );
  actionViewDirView->setChecked( false );
}


void K3bMainWindow::slotCurrentDocChanged( QWidget* w )
{
  if( w->inherits( "K3bView" ) ) {
    // activate actions for file-handling
    actionFileClose->setEnabled( true );
    actionFileSave->setEnabled( true );
    actionFileSaveAs->setEnabled( true );
    actionFileExport->setEnabled( true );
  }
  else {
    // the active window does not represent a file (e.g. the copy-widget)
    actionFileClose->setEnabled( false );
    actionFileSave->setEnabled( false );
    actionFileSaveAs->setEnabled( false );
  }
}


QString K3bMainWindow::findTempFile( const QString& ending, const QString& d )
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


bool K3bMainWindow::eject()
{
  config()->setGroup( "General Options" );
  return config()->readBoolEntry( "Eject when finished", true );
}


void K3bMainWindow::slotJobFinished( K3bJob* job )
{
  job->disconnect();
  delete job;
}


void K3bMainWindow::slotErrorMessage(const QString& message)
{
  KMessageBox::error( this, message );
}


void K3bMainWindow::slotWarningMessage(const QString& message)
{
  KMessageBox::sorry( this, message );
}


void K3bMainWindow::slotCdInfo()
{
  K3bCdInfoDialog* d = new K3bCdInfoDialog( this, "cdinfod" );
  d->show();  // will delete itself (modeless)
}


void K3bMainWindow::slotBlankCdrw()
{
  // K3bBlankingDialog is modeless so don't use exec!
  // the dialog also does a delayed self-destrcut
  K3bBlankingDialog* d = new K3bBlankingDialog( this, "blankingdialog" );
  d->show();
}
