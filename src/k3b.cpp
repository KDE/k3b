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
#include <kstddirs.h>
#include <krun.h>
#include <kurl.h>
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <kglobalsettings.h>
#include <kdialog.h>
#include <kedittoolbar.h>
#include <ksystemtray.h>

#include <stdlib.h>

// application specific includes
#include "k3b.h"
#include "tools/k3bglobals.h"
#include "k3bview.h"
#include "k3bdirview.h"
#include "audio/k3baudiodoc.h"
#include "audio/k3baudioview.h"
#include "device/k3bdevicemanager.h"
#include "device/k3bdevicewidget.h"
#include "audio/k3baudiotrackdialog.h"
#include "option/k3boptiondialog.h"
#include "k3bprojectburndialog.h"
#include "data/k3bdatadoc.h"
#include "data/k3bdataview.h"
#include "data/k3bdatajob.h"
#include "k3bblankingdialog.h"
#include "data/k3bisoimagewritingdialog.h"
#include "tools/k3bexternalbinmanager.h"
#include "k3bprojecttabwidget.h"
#include "rip/songdb/k3bsongmanager.h"
#include "k3baudioplayer.h"
#include "cdcopy/k3bcdcopydialog.h"
#include "dvd/k3bdvdview.h"
#include "k3btempdirselectionwidget.h"
#include "k3bbusywidget.h"

#include "libmad/mad.h"


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
  setPlainCaption( i18n("K3b - The CD Kreator") );

  m_config = kapp->config();
  untitledCount = 0;
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
  actionFileExport->setEnabled( false );
  actionProjectAddFiles->setEnabled( false );

  m_optionDialog = 0;

  // since the icons are not that good activate the text on the toolbar
  toolBar()->setIconText( KToolBar::IconTextRight );
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

  actionProjectAddFiles = new KAction( i18n("&Add Files..."), "filenew", 0, this, SLOT(slotProjectAddFiles()), 
				       actionCollection(), "project_add_files");

  actionViewDirView = new KToggleAction(i18n("Show Directories"), 0, this, SLOT(slotShowDirView()), 
				  actionCollection(), "view_dir");

  actionViewAudioPlayer = new KToggleAction(i18n("Show Audio Player"), 0, this, SLOT(slotViewAudioPlayer()), 
					    actionCollection(), "view_audio_player");

  actionToolsBlankCdrw = new KAction(i18n("&Blank CD-RW"), "cdrwblank", 0, this, SLOT(slotBlankCdrw()),
			       actionCollection(), "tools_blank_cdrw" );
  actionToolsDivxEncoding = new KAction(i18n("&Encode video"),"gear", 0, this, SLOT( slotDivxEncoding() ),
			    actionCollection(), "tools_encode_video");
  actionToolsWriteIsoImage = new KAction(i18n("&Write Iso image"), "gear", 0, this, SLOT(slotWriteIsoImage()),
					 actionCollection(), "tools_write_iso" );

  actionCdCopy = new KAction(i18n("&Copy CD"), "cdcopy", 0, this, SLOT(slotCdCopy()),
			     actionCollection(), "tools_copy_cd" );

  actionSettingsK3bSetup = new KAction(i18n("K3b &Setup"), "configure", 0, this, SLOT(slotK3bSetup()), 
				       actionCollection(), "settings_k3bsetup" );


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
  m_busyWidget = new K3bBusyWidget( statusBar() );

  statusBar()->insertItem( "               ", 100, 1 ); // for showing some info
  statusBar()->addWidget( m_busyWidget, 0, true );
  statusBar()->insertFixedItem( QString("K3b %1").arg(VERSION), 0, true );

  statusBar()->setItemAlignment( 100, Qt::AlignVCenter|AlignLeft );
}


void K3bMainWindow::showBusyInfo( const QString& str )
{
  statusBar()->changeItem( str, 100 );
  m_busyWidget->showBusy( true );
}


void K3bMainWindow::endBusy()
{
  statusBar()->changeItem( "               ", 100 );
  m_busyWidget->showBusy( false );
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
  // create styled document box
//   QFrame* documentBox = new QFrame( mainDock );
//   documentBox->setFrameStyle( QFrame::Box | QFrame::Plain );
//   documentBox->setLineWidth( 5 );
//   QVBoxLayout* documentLayout = new QVBoxLayout( documentBox );
//   documentLayout->setAutoAdd( true );
//   documentLayout->setMargin( 5 );

//   QLabel* projectHeader = new QLabel( documentBox );
//   projectHeader->setText( i18n("Current Projects") );
//   //  projectHeader->setAlignment( AlignHCenter | AlignVCenter );
//   projectHeader->setFont( KGlobalSettings::windowTitleFont() );
//   projectHeader->setMargin( 2 );

//   QPalette oldPal( documentBox->palette() );
//   QPalette p( documentBox->palette() );
//   p.setColor( QColorGroup::Foreground, KGlobalSettings::activeTitleColor() );
//   documentBox->setPalette( p );
//   p.setColor( QColorGroup::Background, KGlobalSettings::activeTitleColor() );
//   p.setColor( QColorGroup::Foreground, KGlobalSettings::activeTextColor() );
//   projectHeader->setPalette( p );

//   QFrame* documentHull = new QFrame( documentBox );
//   documentHull->setFrameStyle( QFrame::Box | QFrame::Plain );
//   documentHull->setLineWidth( 5 );
//   QHBoxLayout* hullLayout = new QHBoxLayout( documentHull );
//   hullLayout->setAutoAdd( true );
//   hullLayout->setMargin( 10 );

  QWidget* documentHull = new QWidget( mainDock );
  QGridLayout* documentHullLayout = new QGridLayout( documentHull );
  documentHullLayout->setMargin( 0 );
  documentHullLayout->setSpacing( 0 );

  QLabel* leftDocPicLabel = new QLabel( documentHull );
  QLabel* centerDocLabel = new QLabel( documentHull );
  QLabel* rightDocPicLabel = new QLabel( documentHull );

  leftDocPicLabel->setPixmap( DesktopIcon( "k3b" ) );
  rightDocPicLabel->setPixmap( DesktopIcon( "k3b" ) );
  centerDocLabel->setText( i18n("Current Projects") );
  centerDocLabel->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  centerDocLabel->setPaletteBackgroundColor( QColor(197, 0, 49) );
  centerDocLabel->setPaletteForegroundColor( Qt::white );
  QFont f(centerDocLabel->font());
  f.setBold(true);
  centerDocLabel->setFont(f);

  // add the document tab to the styled document box
  m_documentTab = new K3bProjectTabWidget( documentHull );
  //  m_documentTab->setPalette( oldPal );

  documentHullLayout->addWidget( leftDocPicLabel, 0, 0 );
  documentHullLayout->addWidget( centerDocLabel, 0, 1 );
  documentHullLayout->addWidget( rightDocPicLabel, 0, 2 );
  documentHullLayout->addMultiCellWidget( m_documentTab, 1, 1, 0, 2 );
  documentHullLayout->setColStretch( 1, 1 );

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


  // ///////////////////////////
  // HACK needed because otherwise I get undefined references ???? :-((
  // //////////////////////////
  delete (new K3bDeviceWidget( deviceManager(), 0 ));

  mad_header* m_madHeader = new mad_header;
  mad_header_init( m_madHeader );
  mad_header_finish( m_madHeader );
  delete m_madHeader;
  mad_synth* m_madSynth  = new mad_synth;
  mad_synth_init( m_madSynth );
  mad_synth_finish( m_madSynth );
  delete m_madSynth;
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
  m_config->writeEntry("ToolBarPos", (int) toolBar("mainToolBar")->barPos());
  actionFileOpenRecent->saveEntries(m_config,"Recent Files");

  // save dock positions!
  manager()->writeConfig( m_config, "Docking Config" );

  m_config->setGroup( "External Programs" );
  m_externalBinManager->saveConfig( m_config );
  m_deviceManager->saveConfig( m_config );

  emit saveConfig( config() );
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
    if( doc->URL().fileName().contains(i18n("Untitled")) )
      fileSaveAs( doc );
    else
      if( !doc->saveDocument(doc->URL()) )
	KMessageBox::error (this,i18n("Could not save the current document !"), i18n("I/O Error !"));
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
  if( K3bAudioView* m = dynamic_cast<K3bAudioView*>( activeView() ) ) {
    QString file = KFileDialog::getSaveFileName( QDir::home().absPath(), "*.toc", k3bMain(), i18n("Export to cdrdao-toc-file") );
    if( !file.isEmpty() ) {
      if( !((K3bAudioDoc*)m->getDocument())->writeTOC( file ) )
	KMessageBox::error( this, i18n("Could not write to file %1").arg( file ), i18n("I/O Error") );
    }
  }
  else if( K3bDataView* m = dynamic_cast<K3bDataView*>( activeView() ) ) {
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

  if( K3bView* view = activeView() )
    {
      view->close(true);
    }

  if( pDocList->isEmpty() ) {
    actionFileSave->setEnabled(false);
    actionFileSaveAs->setEnabled(false);
    actionFileBurn->setEnabled( false );
    actionFileExport->setEnabled( false );
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


void K3bMainWindow::slotSettingsConfigure()
{
  if( !m_optionDialog )
    m_optionDialog = new K3bOptionDialog( this, "SettingsDialog", true );
		
  if( !m_optionDialog->isVisible() ) {
    m_optionDialog->exec();

    // emit a changed signal everytime since we do not know if the user selected
    // "apply" and "cancel" or "ok"
    emit configChanged( m_config );
  }
}


void K3bMainWindow::showOptionDialog( int index )
{
  if( !m_optionDialog )
    m_optionDialog = new K3bOptionDialog( this, "SettingsDialog", true );

  m_optionDialog->showPage( index );
	
  if( !m_optionDialog->isVisible() )
    m_optionDialog->show();
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
  doc->setVolumeID( QString("Data_%1").arg(untitledCount) );

  // create the window
  createClient(doc);
}

void K3bMainWindow::slotDivxEncoding(){
  slotStatusMsg(i18n("Creating new DVD Project."));
   K3bDvdView d( this, "divx");
   d.exec();
}

void K3bMainWindow::slotFileBurn()
{
  K3bView* view = activeView();

  if( view ) {
    K3bDoc* doc = view->getDocument();
    
    if( doc ) {
      // test if there is something to burn
      if( doc->numOfTracks() == 0 ) {
	KMessageBox::information( kapp->mainWidget(), "There is nothing to burn!", "So what?", QString::null, false );
      }
      else {
	K3bProjectBurnDialog* d = view->burnDialog();
	d->exec(true);
	delete d;
      }
    }
  }
}


void K3bMainWindow::init()
{
  emit initializationInfo( i18n("Reading Options...") );


  // this is a little not to hard hack to ensure that we get the "global" k3b appdir
  // k3bui.rc should always be in $KDEDIR/share/apps/k3b/
  QString globalConfigDir = KGlobal::dirs()->findResourceDir( "data", "k3b/k3bui.rc" ) + "k3b";
  QString globalConfigFile =  globalConfigDir + "/k3bsetup";
  KConfig globalConfig( globalConfigFile );

  readOptions();

  // external bin manager
  // ===============================================================================
  emit initializationInfo( i18n("Searching for external programs...") );

  m_externalBinManager = new K3bExternalBinManager( this );
  m_externalBinManager->search();

  if( globalConfig.hasGroup("External Programs") ) {
    globalConfig.setGroup( "External Programs" );
    m_externalBinManager->readConfig( &globalConfig );
  }

  if( config()->hasGroup("External Programs") ) {
    config()->setGroup( "External Programs" );
    m_externalBinManager->readConfig( config() );
  }
  // not thread/kprocess safe
  //m_externalBinManager->checkVersions();

  // ===============================================================================


  // device manager
  // ===============================================================================
  emit initializationInfo( i18n("Scanning for cd devices...") );

  m_deviceManager = new K3bDeviceManager( m_externalBinManager, this );

  if( !m_deviceManager->scanbus() )
    qDebug( "No Devices found!" );

  if( globalConfig.hasGroup("Devices") ) {
    globalConfig.setGroup( "Devices" );
    m_deviceManager->readConfig( &globalConfig );
  }

  if( config()->hasGroup("Devices") ) {
    config()->setGroup( "Devices" );
    m_deviceManager->readConfig( config() );
  }
			
  m_deviceManager->printDevices();
  // ===============================================================================

  emit initializationInfo( i18n("Initializing cd view...") );

  m_dirView->setupFinalize( m_deviceManager );

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
}


void K3bMainWindow::slotEditToolbars()
{
  KEditToolbar dlg(actionCollection());

  if (dlg.exec())
    createGUI();
}


QString K3bMainWindow::findTempFile( const QString& ending, const QString& d )
{
  KURL url(d);
  if( d.isEmpty() ) {
    config()->setGroup( "General Options" );
    url = config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) );
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


void K3bMainWindow::slotProjectAddFiles()
{
  K3bDoc* doc = activeDoc();

  if( doc ) {
    QStringList urls = KFileDialog::getOpenFileNames( ".", "*", this, i18n("Select files to add to the project") );
    if( !urls.isEmpty() )
      doc->addUrls( urls );
  }
  else
    KMessageBox::error( this, i18n("Please create a project before adding files"), i18n("No active Project"));
}


void K3bMainWindow::slotK3bSetup()
{
  KRun::runCommand( "kdesu k3bsetup" );
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


#include "k3b.moc"
