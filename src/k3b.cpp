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
#include <qprinter.h>
#include <qvbox.h>
#include <qwhatsthis.h>
#include <qtooltip.h>
#include <qtoolbutton.h>
#include <qstring.h>
#include <qsplitter.h>
#include <qevent.h>

// include files for KDE
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>

// application specific includes
#include "k3b.h"
#include "k3bview.h"
#include "k3bdirview.h"
#include "k3baudiodoc.h"


K3bApp::K3bApp():KMainWindow(0,"K3b")
{
  config=kapp->config();
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
}

K3bApp::~K3bApp()
{
}

void K3bApp::initActions()
{
  fileNew = KStdAction::openNew(this, SLOT(slotFileNew()), actionCollection());
  fileOpen = KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
  fileOpenRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const KURL&)), actionCollection());
  fileSave = KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
  fileSaveAs = KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  fileClose = KStdAction::close(this, SLOT(slotFileClose()), actionCollection());
  fileQuit = KStdAction::quit(this, SLOT(slotFileQuit()), actionCollection());
  viewToolBar = KStdAction::showToolbar(this, SLOT(slotViewToolBar()), actionCollection());
  viewStatusBar = KStdAction::showStatusbar(this, SLOT(slotViewStatusBar()), actionCollection());

  viewDirView = new KToggleAction(i18n("Show Directories"), 0, this, SLOT(slotShowDirView()), actionCollection(), "view_dir");

  windowTile = new KAction(i18n("&Tile"), 0, this, SLOT(slotWindowTile()), actionCollection(),"window_tile");
  windowCascade = new KAction(i18n("&Cascade"), 0, this, SLOT(slotWindowCascade()), actionCollection(),"window_cascade");

  fileNew->setStatusText(i18n("Creates a new document"));
  fileOpen->setStatusText(i18n("Opens an existing document"));
  fileOpenRecent->setStatusText(i18n("Opens a recently used file"));
  fileSave->setStatusText(i18n("Saves the actual document"));
  fileSaveAs->setStatusText(i18n("Saves the actual document as..."));
  fileClose->setStatusText(i18n("Closes the actual document"));
  fileQuit->setStatusText(i18n("Quits the application"));

  viewToolBar->setStatusText(i18n("Enables/disables the toolbar"));
  viewStatusBar->setStatusText(i18n("Enables/disables the statusbar"));

  viewDirView->setChecked( true );

  windowMenu = new KActionMenu(i18n("&Window"), actionCollection(), "window_menu");
  connect(windowMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(windowMenuAboutToShow()));

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
  QSplitter* view_back = new QSplitter( this );
  view_back->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  pWorkspace = new QWorkspace( view_back );
  m_dirView = new K3bDirView( view_back );
  setCentralWidget(view_back);
}


void K3bApp::createClient(K3bDoc* doc)
{
  K3bView* w = doc->newView( pWorkspace );
  w->installEventFilter(this);
  doc->addView(w);
  w->setIcon(kapp->miniIcon());
  w->show();
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

  doc = new K3bAudioDoc();
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
  config->setGroup("General Options");
  config->writeEntry("Geometry", size());
  config->writeEntry("Show Toolbar", toolBar()->isVisible());
  config->writeEntry("Show Statusbar",statusBar()->isVisible());
  config->writeEntry("ToolBarPos", (int) toolBar("mainToolBar")->barPos());
  fileOpenRecent->saveEntries(config,"Recent Files");
}


void K3bApp::readOptions()
{
	
  config->setGroup("General Options");

  // bar status settings
  bool bViewToolbar = config->readBoolEntry("Show Toolbar", true);
  viewToolBar->setChecked(bViewToolbar);
  slotViewToolBar();

  bool bViewStatusbar = config->readBoolEntry("Show Statusbar", true);
  viewStatusBar->setChecked(bViewStatusbar);
  slotViewStatusBar();

  // bar position settings
  KToolBar::BarPosition toolBarPos;
  toolBarPos=(KToolBar::BarPosition) config->readNumEntry("ToolBarPos", KToolBar::Top);
  toolBar("mainToolBar")->setBarPos(toolBarPos);

  // initialize the recent file list
  fileOpenRecent->loadEntries(config,"Recent Files");

  QSize size=config->readSizeEntry("Geometry");
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
    K3bDoc* pDoc=pView->getDocument();
    if(pDoc->canCloseFrame(pView))
    {
      pDoc->removeView(pView);
      if(!pDoc->firstView())
        pDocList->remove(pDoc);
      e->accept();
    }
    else
      e->ignore();
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

  K3bView* m = (K3bView*)pWorkspace->activeWindow();
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
    K3bView* m = (K3bView*)pWorkspace->activeWindow();
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
  K3bView* m = (K3bView*)pWorkspace->activeWindow();
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
  saveOptions();
  // close the first window, the list makes the next one the first again.
  // This ensures that queryClose() is called on each window to ask for closing
  KMainWindow* w;
  if(memberList)
  {
    for(w=memberList->first(); w!=0; w=memberList->first())
    {
      // only close the window if the closeEvent is accepted. If the user presses Cancel on the saveModified() dialog,
      // the window and the application stay open.
      if(!w->close())
        break;
    }
  }	
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


void K3bApp::slotWindowTile(){
  pWorkspace->tile();
}

void K3bApp::slotWindowCascade(){
  pWorkspace->cascade();
}

void K3bApp::slotStatusMsg(const QString &text)
{
  ///////////////////////////////////////////////////////////////////
  // change status message permanently
  statusBar()->clear();
  statusBar()->changeItem(text,1);
}


void K3bApp::windowMenuAboutToShow()
{
  windowMenu->popupMenu()->clear();
  windowMenu->insert(windowCascade);
  windowMenu->insert(windowTile);

  if ( pWorkspace->windowList().isEmpty() ){
    windowCascade->setEnabled(false);
    windowTile->setEnabled(false);
  }
  else{
    windowCascade->setEnabled(true);
    windowTile->setEnabled(true);
  }
  windowMenu->popupMenu()->insertSeparator();

  QWidgetList windows = pWorkspace->windowList();
  for ( int i = 0; i < int(windows.count()); ++i )
  {
    int id = windowMenu->popupMenu()->insertItem(QString("&%1 ").arg(i+1)+windows.at(i)->caption(), this, SLOT( windowMenuActivated( int ) ) );
    windowMenu->popupMenu()->setItemParameter( id, i );
    windowMenu->popupMenu()->setItemChecked( id, pWorkspace->activeWindow() == windows.at(i) );
  }
}

void K3bApp::windowMenuActivated( int id )
{
  QWidget* w = pWorkspace->windowList().at( id );
  if ( w )
    w->setFocus();
}


void K3bApp::slotShowDirView(){
	if( !m_dirView->isVisible() )
		m_dirView->show();
	else
		m_dirView->hide();
}
