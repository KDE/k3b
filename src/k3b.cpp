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
#include "k3bview.h"
#include "k3bdirview.h"
#include "k3baudiodoc.h"
#include "k3bdevicemanager.h"
#include "k3baudiotrackdialog.h"
#include "k3bcopywidget.h"
#include "k3bripperwidget.h"
#include "k3boptiondialog.h"


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

  m_audioTrackDialog = 0;
  m_optionDialog = 0;
}

K3bApp::~K3bApp()
{
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

  fileBurn = new KAction( i18n("&Burn..."), 0, this, SLOT(slotFileBurn()), actionCollection(), "file_burn");

  fileNewMenu = new KActionMenu( i18n("&New Project"), SmallIconSet("filenew"), actionCollection(), "file_new" );
  fileNewAudio = new KAction(i18n("New &Audio project"), SmallIconSet("filenew"), 0, this, SLOT(newAudioDoc()), actionCollection(), "file_new_audio");
  fileNewData = new KAction(i18n("New &Data project"), SmallIconSet("filenew"), 0, this, SLOT(newDataDoc()), actionCollection(), "file_new_data");
  fileNewMenu->insert( fileNewAudio );
  fileNewMenu->insert( fileNewData );
  fileNewMenu->setDelayed( false );

  viewDirView = new KToggleAction(i18n("Show Directories"), 0, this, SLOT(slotShowDirView()), actionCollection(), "view_dir");

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
  ////////////////////////////////////////////////////////////////////
  // here the main view of the KMainWindow is created by a background box and
  // the QWorkspace instance for MDI view.
  mainDock = createDockWidget( "Workspace", SmallIcon("idea") );
  setView( mainDock );
  setMainDockWidget( mainDock );
  mainDock->setEnableDocking( KDockWidget::DockNone );

//  pWorkspace = new QWorkspace( mainDock );
//  mainDock->setWidget( pWorkspace );

  m_documentTab = new QTabWidget( mainDock );
  mainDock->setWidget( m_documentTab );
  connect( m_documentTab, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotCurrentDocChanged(QWidget*)) );

  // add the cd-copy-widget to the tab
  m_documentTab->addTab( new K3bCopyWidget( m_documentTab ), "&Copy CD" );
  m_documentTab->addTab( new K3bRipperWidget( m_documentTab ), "&Ripping" );

  dirDock = createDockWidget( "DirDock", SmallIcon("idea") );
  m_dirView = new K3bDirView( dirDock );
  dirDock->setWidget( m_dirView );
  dirDock->setEnableDocking( KDockWidget::DockCorner );
  connect( dirDock, SIGNAL(headerCloseButtonClicked()), this, SLOT(slotDirDockHidden()) );

  // dock it!
  dirDock->manualDock( mainDock, KDockWidget::DockLeft, 30 );
}


void K3bApp::createClient(K3bDoc* doc)
{
  K3bView* w = doc->newView( m_documentTab );
  w->installEventFilter(this);
  doc->addView(w);
  w->setIcon(kapp->miniIcon());
  m_documentTab->addTab( w, w->caption() );

  fileBurn->setEnabled( true );
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

  doc = new K3bAudioDoc( this );
  pDocList->append(doc);
  doc->newDocument();
  // Creates an untitled window if file is 0	
  if(url.isEmpty())
  {
    untitledCount+=1;
    QString fileName=QString(i18n("Untitled%1")).arg(untitledCount);
    KURL url;
    url.setFileName(fileName);
    doc->setURL(url);
  }
  // Open the file
  else
  {
    if(!doc->openDocument(url))
    {
      KMessageBox::error (this,i18n("Could not open document !"), i18n("Error !"));
      delete doc;
      return;	
    }
	  fileOpenRecent->addURL(url);
  }
  // create the window
  createClient(doc);

  slotStatusMsg(i18n("Ready."));
}


void K3bApp::saveOptions()
{	
qDebug("Saving options!");

  m_config->setGroup("General Options");
  m_config->writeEntry("Geometry", size());
  m_config->writeEntry("Show Toolbar", toolBar()->isVisible());
  m_config->writeEntry("Show Statusbar",statusBar()->isVisible());
  m_config->writeEntry("ToolBarPos", (int) toolBar("mainToolBar")->barPos());
  fileOpenRecent->saveEntries(m_config,"Recent Files");

  m_config->writeEntry("Temp Dir", "/usr/cdburn/");
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
}

void K3bApp::saveProperties(KConfig *_cfg)
{

}


void K3bApp::readProperties(KConfig* _cfg)
{
}

bool K3bApp::queryClose()
{
  QStringList saveFiles;
  K3bDoc* doc;
  if(pDocList->isEmpty())
    return true;

  for(doc=pDocList->first(); doc!=0;doc=pDocList->next())
  {
    if(doc->isModified())
      saveFiles.append(doc->URL().fileName());
  }
  if(saveFiles.isEmpty())
    return true;

  switch (KMessageBox::questionYesNoList(this,
          i18n("One or more documents have been modified.\nSave changes before exiting?"),saveFiles))
  {
  case KMessageBox::Yes:
    for(doc=pDocList->first(); doc!=0;doc=pDocList->next())
    {
      if(doc->URL().fileName().contains(i18n("Untitled")))
        slotFileSaveAs();
      else
      {
        if(!doc->saveDocument(doc->URL()))
        {
          KMessageBox::error (this,i18n("Could not save the current document !"), i18n("I/O Error !"));
          return false;
         }
       }
     }
     return true;
  case KMessageBox::No:
  default:
  return true;
  }
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
//      setWndTitle(m);
	    fileOpenRecent->addURL(url);
    }	
  }

  slotStatusMsg(i18n("Ready."));
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

  slotStatusMsg(i18n("Ready."));
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
		dirDock->show();
	}
	else {
		dirDock->hide();
		dirDock->undock();
	}
}


void K3bApp::slotSettingsConfigure()
{
	if( !m_optionDialog )
		m_optionDialog = new K3bOptionDialog( this, "SettingsDialog", true );
		
	if( !m_optionDialog->isVisible() )
		m_optionDialog->exec();
}

void K3bApp::searchExternalProgs()
{
	m_config->setGroup("External Programs");
	
	if( !m_config->hasKey("cdrecord path") ) {
		if( QFile::exists( "/usr/bin/cdrecord" ) ) {
			m_cdrecord = "/usr/bin/cdrecord";
			qDebug("(K3bApp) found cdrecord in " + m_cdrecord );
		}
		else if( QFile::exists( "/usr/local/bin/cdrecord" ) ) {
			m_cdrecord = "/usr/local/bin/cdrecord";
			qDebug("(K3bApp) found cdrecord in " + m_cdrecord );
		}
		else {
			bool ok = true;
			while( !QFile::exists( m_cdrecord ) && ok )
				m_cdrecord = KLineEditDlg::getText( "Could not find cdrecord. Please insert the full path...", "cdrecord", &ok, this );
		}
		m_config->writeEntry( "cdrecord path", m_cdrecord );
	}
	
	if( !m_config->hasKey( "mpg123 path" ) ) {
		if( QFile::exists( "/usr/bin/mpg123" ) ) {
			m_mpg123 = "/usr/bin/mpg123";
			qDebug("(K3bApp) found mpg123 in " + m_cdrecord );
		}
		else if( QFile::exists( "/usr/local/bin/mpg123" ) ) {
			m_mpg123 = "/usr/local/bin/mpg123";
			qDebug("(K3bApp) found mpg123 in " + m_cdrecord );
		}
		else {
			bool ok = true;
			while( !QFile::exists( m_mpg123 ) && ok )
				m_mpg123 = KLineEditDlg::getText( "Could not find mpg123. Please insert the full path...", "mpg123", &ok, this );
		}
		m_config->writeEntry( "mpg123 path", m_mpg123 );
	}
	
	m_config->sync();
}

void K3bApp::newAudioDoc()
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

void K3bApp::newDataDoc()
{
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
		if( w->inherits( "K3bView" ) ) {
			K3bDoc* doc = ((K3bView*)w)->getDocument();
			doc->showBurnDialog();
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
  m_deviceManager->printDevices();
}


void K3bApp::slotDirDockHidden()
{
	// check if that is really true
	if( !dirDock->isVisible() )
		viewDirView->setChecked( false );
}


void K3bApp::slotCurrentDocChanged( QWidget* w )
{
	if( w->inherits( "K3bView" ) ) {
		// activate actions for file-handling
		fileClose->setEnabled( true );
		fileSave->setEnabled( true );
		fileSaveAs->setEnabled( true );
	}
	else {
		// the active window does not represent a file (e.g. the copy-widget)
		fileClose->setEnabled( false );
		fileSave->setEnabled( false );
		fileSaveAs->setEnabled( false );
	}
}
