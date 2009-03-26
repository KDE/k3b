/*
 *
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config-k3b.h>


// include files for QT
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QLayout>
#include <QList>
#include <QString>
#include <QTimer>

// include files for KDE
#include <kaboutdata.h>
#include <KActionMenu>
#include <KConfig>
#include <KEditToolBar>
#include <KFileDialog>
#include <kfileplacesmodel.h>
#include <KGlobal>
#include <KMessageBox>
#include <KMenuBar>
#include <KProcess>
#include <KRecentDocument>
#include <KRecentFilesAction>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KStandardDirs>
#include <KStatusBar>
#include <ktip.h>
#include <KToggleAction>
#include <KUrl>
#include <KXMLGUIFactory>
#include <kio/netaccess.h>
#include <kio/deletejob.h>
#include <cstdlib>

// application specific includes
#include "k3b.h"
#include "k3baction.h"
#include "k3bappdevicemanager.h"
#include "k3bapplication.h"
#include "k3baudiodecoder.h"
#include "k3baudiodoc.h"
#include "k3baudiotrackdialog.h"
#include "k3baudioview.h"
#include "k3bcuefileparser.h"
#include "k3bdatadoc.h"
#include "k3bdataview.h"
#include "k3bdeviceselectiondialog.h"
#include "k3bdirview.h"
#include "k3bexternalbinmanager.h"
#include "k3bfiletreeview.h"
#include "k3bglobals.h"
#include "k3biso9660.h"
#include "k3bjob.h"
#include "k3bmediacache.h"
#include "k3bmediaselectiondialog.h"
#include "k3bmedium.h"
#include "k3bmixeddoc.h"
#include "k3bmixedview.h"
#include "k3bmovixdoc.h"
#include "k3bmovixview.h"
#include "k3bprojectburndialog.h"
#include "k3bpassivepopup.h"
#include "k3bplugin.h"
#include "k3bpluginmanager.h"
#include "k3bprojectmanager.h"
#include "k3bprojecttabwidget.h"
#include "k3bsidepanel.h"
#include "k3bsignalwaiter.h"
#include "k3bstdguiitems.h"
#include "k3bsystemproblemdialog.h"
#include "k3bstatusbarmanager.h"
#include "k3btempdirselectionwidget.h"
#include "k3bthemedheader.h"
#include "k3bthememanager.h"
#include "k3burlnavigator.h"
#include "k3bvcddoc.h"
#include "k3bvcdview.h"
#include "k3bvideodvddoc.h"
#include "k3bvideodvdview.h"
#include "k3bview.h"
#include "k3bwelcomewidget.h"
#include "misc/k3bimagewritingdialog.h"
#include "misc/k3bmediacopydialog.h"
#include "misc/k3bmediaformattingdialog.h"
#include "option/k3boptiondialog.h"
#include "projects/k3bdatamultisessionimportdialog.h"


class K3b::MainWindow::Private
{
public:
    K3b::Doc* lastDoc;

    K3b::WelcomeWidget* welcomeWidget;
    QWidget* documentHull;
};


K3b::MainWindow::MainWindow()
    : KXmlGuiWindow(0)
{
    //setup splitter behavior
    //manager()->setSplitterHighResolution(true);
    //manager()->setSplitterOpaqueResize(true);
    //manager()->setSplitterKeepSize(true);

    d = new Private;
    d->lastDoc = 0;

    setPlainCaption( i18n("K3b - The CD and DVD Kreator") );

    // /////////////////////////////////////////////////////////////////
    // call inits to invoke all other construction parts
    initActions();
    initView();
    initStatusBar();
    createGUI();

    // /////////////////////////////////////////////////////////////////
    // incorporate Device Manager into main window
    factory()->addClient( k3bappcore->appDeviceManager() );
    connect( k3bappcore->appDeviceManager(), SIGNAL(detectingDiskInfo(K3b::Device::Device*)),
             this, SLOT(showDiskInfo(K3b::Device::Device*)) );

    // we need the actions for the welcomewidget
    KConfigGroup grp( config(), "Welcome Widget" );
    d->welcomeWidget->loadConfig( grp );

    // fill the tabs action menu
    m_documentTab->insertAction( actionFileSave );
    m_documentTab->insertAction( actionFileSaveAs );
    m_documentTab->insertAction( actionFileClose );

    // /////////////////////////////////////////////////////////////////
    // disable actions at startup
    slotStateChanged( "state_project_active", KXMLGUIClient::StateReverse );

    connect( k3bappcore->projectManager(), SIGNAL(newProject(K3b::Doc*)), this, SLOT(createClient(K3b::Doc*)) );
    connect( k3bcore->deviceManager(), SIGNAL(changed()), this, SLOT(slotCheckSystemTimed()) );

    // FIXME: now make sure the welcome screen is displayed completely
    resize( 780, 550 );
//   getMainDockWidget()->resize( getMainDockWidget()->size().expandedTo( d->welcomeWidget->sizeHint() ) );
//   m_dirTreeDock->resize( QSize( m_dirTreeDock->sizeHint().width(), m_dirTreeDock->height() ) );

    readOptions();
}

K3b::MainWindow::~MainWindow()
{
    delete d;
}


KSharedConfig::Ptr K3b::MainWindow::config() const
{
    return KGlobal::config();
}


void K3b::MainWindow::initActions()
{
    // merge in the device actions from the device manager
    // operator+= is deprecated but I know no other way to do this. Why does the KDE app framework
    // need to have all actions in the mainwindow's actioncollection anyway (or am I just to stupid to
    // see the correct solution?)

    actionFileOpen = KStandardAction::open(this, SLOT(slotFileOpen()), actionCollection());
    actionFileOpenRecent = KStandardAction::openRecent(this, SLOT(slotFileOpenRecent(const KUrl&)), actionCollection());
    actionFileSave = KStandardAction::save(this, SLOT(slotFileSave()), actionCollection());
    actionFileSaveAs = KStandardAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
    actionFileSaveAll = K3b::createAction(this, i18n("Save All"), "document-save-all", 0, this, SLOT(slotFileSaveAll()),
                                          actionCollection(), "file_save_all" );
    actionFileClose = KStandardAction::close(this, SLOT(slotFileClose()), actionCollection());
    actionFileCloseAll = K3b::createAction(this, i18n("Close All"), 0, 0, this, SLOT(slotFileCloseAll()),
                                           actionCollection(), "file_close_all" );
    actionFileQuit = KStandardAction::quit(this, SLOT(slotFileQuit()), actionCollection());
    actionViewStatusBar = KStandardAction::showStatusbar(this, SLOT(slotViewStatusBar()), actionCollection());
    actionSettingsConfigure = KStandardAction::preferences(this, SLOT(slotSettingsConfigure()), actionCollection() );

    // the tip action
    (void)KStandardAction::tipOfDay(this, SLOT(slotShowTips()), actionCollection() );
    (void)KStandardAction::keyBindings( this, SLOT( slotConfigureKeys() ), actionCollection() );

    KStandardAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
    setStandardToolBarMenuEnabled(true);
    KStandardAction::showMenubar( this, SLOT(slotShowMenuBar()), actionCollection() );

    //FIXME kde4 verify it
    actionFileNewMenu = new KActionMenu( i18n("&New Project"),this );
    actionFileNewMenu->setIcon( KIcon( "document-new" ) );
    actionCollection()->addAction( "file_new", actionFileNewMenu );
    actionFileNewAudio = K3b::createAction(this,i18n("New &Audio CD Project"), "audiocd", 0, this, SLOT(slotNewAudioDoc()),
                                           actionCollection(), "file_new_audio");
    actionFileNewData = K3b::createAction(this,i18n("New &Data Project"), "datacd", 0, this, SLOT(slotNewDataDoc()),
                                          actionCollection(), "file_new_data");
    actionFileNewMixed = K3b::createAction(this,i18n("New &Mixed Mode CD Project"), "mixedcd", 0, this, SLOT(slotNewMixedDoc()),
                                           actionCollection(), "file_new_mixed");
    actionFileNewVcd = K3b::createAction(this,i18n("New &Video CD Project"), "videocd", 0, this, SLOT(slotNewVcdDoc()),
                                         actionCollection(), "file_new_vcd");
    actionFileNewMovix = K3b::createAction(this,i18n("New &eMovix Project"), "emovix", 0, this, SLOT(slotNewMovixDoc()),
                                           actionCollection(), "file_new_movix");
    actionFileNewVideoDvd = K3b::createAction(this,i18n("New V&ideo DVD Project"), "videodvd", 0, this, SLOT(slotNewVideoDvdDoc()),
                                              actionCollection(), "file_new_video_dvd");
    actionFileContinueMultisession = K3b::createAction(this,i18n("Continue Multisession Project"), "datacd", 0, this, SLOT(slotContinueMultisession()),
                                                       actionCollection(), "file_continue_multisession" );

    actionFileNewMenu->setDelayed( false );
    actionFileNewMenu->addAction( actionFileNewData );
    actionFileNewMenu->addAction( actionFileContinueMultisession );
    actionFileNewMenu->addSeparator();
    actionFileNewMenu->addAction( actionFileNewAudio );
    actionFileNewMenu->addAction( actionFileNewMixed );
    actionFileNewMenu->addSeparator();
    actionFileNewMenu->addAction( actionFileNewVcd );
    actionFileNewMenu->addAction( actionFileNewVideoDvd );
    actionFileNewMenu->addSeparator();
    actionFileNewMenu->addAction( actionFileNewMovix );





    actionProjectAddFiles = K3b::createAction(this, i18n("&Add Files..."), "document-open", 0, this, SLOT(slotProjectAddFiles()),
                                              actionCollection(), "project_add_files");

    KAction* actionClearProject = K3b::createAction(this,i18n("&Clear Project"), QApplication::isRightToLeft() ? "edit-clear-locationbar-rtl" : "edit-clear-locationbar-ltr", 0,
                                                    this, SLOT(slotClearProject()), actionCollection(), "project_clear_project" );


    actionViewDocumentHeader = new KToggleAction(i18n("Show Document Header"),this);
    QAction *action= actionCollection()->addAction("view_document_header",actionViewDocumentHeader);
    connect( action , SIGNAL(toggled(bool)) , this , SLOT(slotViewDocumentHeader()) );


    KAction* actionToolsFormatMedium = K3b::createAction( this,
                                                          i18n("&Format/Erase rewritable disk..."),
                                                          "formatdvd",
                                                          0,
                                                          this,
                                                          SLOT(slotFormatMedium()),
                                                          actionCollection(),
                                                          "tools_format_medium" );
    actionToolsFormatMedium->setIconText( i18n( "Format" ) );

    actionToolsWriteImage = K3b::createAction( this,
                                               i18n("&Burn Image..."),
                                               "burn_cdimage",
                                               0,
                                               this,
                                               SLOT(slotWriteImage()),
                                               actionCollection(),
                                               "tools_write_image" );

    KAction* actionToolsMediaCopy = K3b::createAction( this,
                                                       i18n("Copy &Medium..."),
                                                       "cdcopy",
                                                       0,
                                                       this,
                                                       SLOT(slotMediaCopy()),
                                                       actionCollection(),
                                                       "tools_copy_medium" );
    actionToolsMediaCopy->setIconText( i18n( "Copy" ) );

    actionToolsCddaRip = K3b::createAction( this,
                                            i18n("Rip Audio CD..."),
                                            "cddarip",
                                            0,
                                            this,
                                            SLOT(slotCddaRip()),
                                            actionCollection(),
                                            "tools_cdda_rip" );
    actionToolsVideoDvdRip = K3b::createAction( this,
                                                i18n("Rip Video DVD..."),
                                                "videodvd",
                                                0,
                                                this,
                                                SLOT(slotVideoDvdRip()),
                                                actionCollection(),
                                                "tools_videodvd_rip" );
    actionToolsVideoCdRip = K3b::createAction( this,
                                               i18n("Rip Video CD..."),
                                               "videocd",
                                               0,
                                               this,
                                               SLOT(slotVideoCdRip()),
                                               actionCollection(),
                                               "tools_videocd_rip" );

    (void)K3b::createAction( this,
                             i18n("System Check"),
                             0,
                             0,
                             this,
                             SLOT(slotManualCheckSystem()),
                             actionCollection(),
                             "help_check_system" );

#ifdef BUILD_K3BSETUP
    actionSettingsK3bSetup = K3b::createAction( this,
                                                i18n("&Setup System Permissions..."),
                                                "configure",
                                                0,
                                                this,
                                                SLOT(slotK3bSetup()),
                                                actionCollection(),
                                                "settings_k3bsetup" );
#endif

#ifdef K3B_DEBUG
    (void)K3b::createAction(this, "Test Media Selection ComboBox", 0, 0, this,
                            SLOT(slotMediaSelectionTester()), actionCollection(),
                            "test_media_selection" );
#endif

    actionFileNewMenu->setToolTip(i18n("Creates a new project"));
    actionFileNewData->setToolTip( i18n("Creates a new data project") );
    actionFileNewAudio->setToolTip( i18n("Creates a new audio CD project") );
    actionFileNewMovix->setToolTip( i18n("Creates a new eMovix project") );
    actionFileNewVcd->setToolTip( i18n("Creates a new Video CD project") );
    actionToolsFormatMedium->setToolTip( i18n("Open the rewritable disk formatting/erasing dialog") );
    actionToolsWriteImage->setToolTip( i18n("Write an Iso9660, cue/bin, or cdrecord clone image to an optical disc") );
    actionToolsMediaCopy->setToolTip( i18n("Open the media copy dialog") );
    actionFileOpen->setToolTip(i18n("Opens an existing project"));
    actionFileOpenRecent->setToolTip(i18n("Opens a recently used file"));
    actionFileSave->setToolTip(i18n("Saves the current project"));
    actionFileSaveAs->setToolTip(i18n("Saves the current project to a new url"));
    actionFileSaveAll->setToolTip(i18n("Saves all open projects"));
    actionFileClose->setToolTip(i18n("Closes the current project"));
    actionFileCloseAll->setToolTip(i18n("Closes all open projects"));
    actionFileQuit->setToolTip(i18n("Quits the application"));
    actionSettingsConfigure->setToolTip( i18n("Configure K3b settings") );
#ifdef BUILD_K3BSETUP
    actionSettingsK3bSetup->setToolTip( i18n("Setup the system permissions (requires root privileges)") );
#endif
    actionToolsCddaRip->setToolTip( i18n("Digitally extract tracks from an audio CD") );
    actionToolsVideoDvdRip->setToolTip( i18n("Transcode Video DVD titles") );
    actionToolsVideoCdRip->setToolTip( i18n("Extract tracks from a Video CD") );
    actionProjectAddFiles->setToolTip( i18n("Add files to the current project") );
    actionClearProject->setToolTip( i18n("Clear the current project") );

    //FIXME kde4
    // make sure the tooltips are used for the menu
    //actionCollection()->setHighlightingEnabled( true );
}



QList<K3b::Doc*> K3b::MainWindow::projects() const
{
    return k3bappcore->projectManager()->projects();
}


void K3b::MainWindow::slotConfigureKeys()
{
    KShortcutsDialog::configure( actionCollection(),KShortcutsEditor::LetterShortcutsDisallowed, this );
}

void K3b::MainWindow::initStatusBar()
{
    m_statusBarManager = new K3b::StatusBarManager( this );
}


void K3b::MainWindow::initView()
{
    setDockOptions( AnimatedDocks );

    // setup main docking things

    // --- Document Dock ----------------------------------------------------------------------------
    d->documentHull = new QWidget( this );
    setCentralWidget( d->documentHull );
    QGridLayout* documentHullLayout = new QGridLayout( d->documentHull );
    documentHullLayout->setMargin( 2 );
    documentHullLayout->setSpacing( 0 );

    m_documentHeader = new K3b::ThemedHeader( d->documentHull );
    m_documentHeader->setTitle( i18n("Current Projects") );
    m_documentHeader->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
    m_documentHeader->setLeftPixmap( K3b::Theme::PROJECT_LEFT );
    m_documentHeader->setRightPixmap( K3b::Theme::PROJECT_RIGHT );

    // add the document tab to the styled document box
    m_documentTab = new K3b::ProjectTabWidget( d->documentHull );

    documentHullLayout->addWidget( m_documentHeader, 0, 0 );
    documentHullLayout->addWidget( m_documentTab, 1, 0 );

    connect( m_documentTab, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotCurrentDocChanged()) );

    d->welcomeWidget = new K3b::WelcomeWidget( this, m_documentTab );
    m_documentTab->addTab( d->welcomeWidget, i18n("Quickstart") );
    // ---------------------------------------------------------------------------------------------

    // --- Directory Dock --------------------------------------------------------------------------
    m_dirTreeDock = new QDockWidget( this );
    m_dirTreeDock->setObjectName("dirtreedock");
    m_dirTreeDock->setFeatures( QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable );
    addDockWidget( Qt::TopDockWidgetArea, m_dirTreeDock );
    QAction *action = m_dirTreeDock->toggleViewAction();
    action->setText(i18n("Show Directories"));
    actionCollection()->addAction( "view_dir_tree", action );

    K3b::FileTreeView* sidePanel = new K3b::FileTreeView( m_dirTreeDock );
    //K3b::SidePanel* sidePanel = new K3b::SidePanel( this, m_dirTreeDock, "sidePanel" );

    m_dirTreeDock->setWidget( sidePanel );
    // ---------------------------------------------------------------------------------------------


    // --- Contents Dock ---------------------------------------------------------------------------
    m_contentsDock = new QDockWidget( this );
    m_contentsDock->setObjectName("contentsdock");
    m_contentsDock->setFeatures( QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable );
    addDockWidget ( Qt::TopDockWidgetArea, m_contentsDock );
    action = m_contentsDock->toggleViewAction();
    action->setText( i18n("Show Contents") );
    actionCollection()->addAction( "view_contents", action );

    m_dirView = new K3b::DirView( sidePanel/*->fileTreeView()*/, m_contentsDock );
    m_contentsDock->setWidget( m_dirView );
    //m_contentsDock->manualDock( m_dirTreeDock, K3DockWidget::DockRight, 2000 );

    // --- filetreecombobox-toolbar ----------------------------------------------------------------
	KFilePlacesModel* filePlacesModel = new KFilePlacesModel;
    K3b::UrlNavigator* urlNavigator = new K3b::UrlNavigator( filePlacesModel, this );
    connect( urlNavigator, SIGNAL(activated(const KUrl&)), m_dirView, SLOT(showUrl(const KUrl& )) );
    connect( urlNavigator, SIGNAL(activated(K3b::Device::Device*)), m_dirView, SLOT(showDevice(K3b::Device::Device* )) );
    connect( m_dirView, SIGNAL(urlEntered(const KUrl&)), urlNavigator, SLOT(setUrl(const KUrl&)) );
    connect( m_dirView, SIGNAL(deviceSelected(K3b::Device::Device*)), urlNavigator, SLOT(setDevice(K3b::Device::Device*)) );
    QWidgetAction * urlNavigatorAction = new QWidgetAction(this);
    urlNavigatorAction->setDefaultWidget(urlNavigator);
    urlNavigatorAction->setText(i18n("&Quick Dir Selector"));
    actionCollection()->addAction( "quick_dir_selector", urlNavigatorAction );
    // ---------------------------------------------------------------------------------------------
}


void K3b::MainWindow::createClient( K3b::Doc* doc )
{
    kDebug();

    // create the proper K3b::View (maybe we should put this into some other class like K3b::ProjectManager)
    K3b::View* view = 0;
    switch( doc->type() ) {
    case K3b::Doc::AUDIO:
        view = new K3b::AudioView( static_cast<K3b::AudioDoc*>(doc), m_documentTab );
        break;
    case K3b::Doc::DATA:
        view = new K3b::DataView( static_cast<K3b::DataDoc*>(doc), m_documentTab );
        break;
    case K3b::Doc::MIXED:
    {
        K3b::MixedDoc* mixedDoc = static_cast<K3b::MixedDoc*>(doc);
        view = new K3b::MixedView( mixedDoc, m_documentTab );
        mixedDoc->dataDoc()->setView( view );
        mixedDoc->audioDoc()->setView( view );
        break;
    }
    case K3b::Doc::VCD:
        view = new K3b::VcdView( static_cast<K3b::VcdDoc*>(doc), m_documentTab );
        break;
    case K3b::Doc::MOVIX:
        view = new K3b::MovixView( static_cast<K3b::MovixDoc*>(doc), m_documentTab );
        break;
    case K3b::Doc::VIDEODVD:
        view = new K3b::VideoDvdView( static_cast<K3b::VideoDvdDoc*>(doc), m_documentTab );
        break;
    }

    doc->setView( view );
    view->setWindowTitle( doc->URL().fileName() );

    m_documentTab->insertTab( doc );
    m_documentTab->setCurrentWidget( view );

    slotCurrentDocChanged();
}


K3b::View* K3b::MainWindow::activeView() const
{
    QWidget* w = m_documentTab->currentWidget();
    if( K3b::View* view = dynamic_cast<K3b::View*>(w) )
        return view;
    else
        return 0;
}


K3b::Doc* K3b::MainWindow::activeDoc() const
{
    if( activeView() )
        return activeView()->getDocument();
    else
        return 0;
}


K3b::Doc* K3b::MainWindow::openDocument(const KUrl& url)
{
    slotStatusMsg(i18n("Opening file..."));

    //
    // First we check if this is an iso image in case someone wants to open one this way
    //
    if( !isCdDvdImageAndIfSoOpenDialog( url ) ) {

        // see if it's an audio cue file
        K3b::CueFileParser parser( url.path() );
        if( parser.isValid() && parser.toc().contentType() == K3b::Device::AUDIO ) {
            K3b::Doc* doc = k3bappcore->projectManager()->createProject( K3b::Doc::AUDIO );
            doc->addUrl( url );
            return doc;
        }
        else {
            // check, if document already open. If yes, set the focus to the first view
            K3b::Doc* doc = k3bappcore->projectManager()->findByUrl( url );
            if( doc ) {
                doc->view()->setFocus();
                return doc;
            }

            doc = k3bappcore->projectManager()->openProject( url );

            if( doc == 0 ) {
                KMessageBox::error (this,i18n("Could not open document."), i18n("Error"));
                return 0;
            }

            actionFileOpenRecent->addUrl(url);

            return doc;
        }
    }
    else
        return 0;
}


void K3b::MainWindow::saveOptions()
{
    KConfigGroup recentGrp(config(),"Recent Files");
    actionFileOpenRecent->saveEntries( recentGrp );

    KConfigGroup grpFileView( config(), "file view" );
    m_dirView->saveConfig( grpFileView );

    KConfigGroup grpWindows(config(), "main_window_settings");
    saveMainWindowSettings( grpWindows );

    k3bcore->saveSettings( config() );

    KConfigGroup grp(config(), "Welcome Widget" );
    d->welcomeWidget->saveConfig( grp );

    KConfigGroup grpOption( config(), "General Options" );
    grpOption.writeEntry( "Show Document Header", actionViewDocumentHeader->isChecked() );

    config()->sync();
}


void K3b::MainWindow::readOptions()
{
    KConfigGroup grp( config(), "General Options" );

    bool bViewDocumentHeader = grp.readEntry("Show Document Header", true);
    actionViewDocumentHeader->setChecked(bViewDocumentHeader);

    // initialize the recent file list
    KConfigGroup recentGrp(config(), "Recent Files");
    actionFileOpenRecent->loadEntries( recentGrp );

    KConfigGroup grpWindow(config(), "main_window_settings");
    applyMainWindowSettings( grpWindow );

    KConfigGroup grpFileView( config(), "file view" );
    m_dirView->readConfig( grpFileView );

    slotViewDocumentHeader();
}


void K3b::MainWindow::saveProperties( KConfigGroup& grp )
{
    // 1. put saved projects in the config
    // 2. save every modified project in  "~/.kde/share/apps/k3b/sessions/" + KApp->sessionId()
    // 3. save the url of the project (might be something like "AudioCD1") in the config
    // 4. save the status of every project (modified/saved)

    QString saveDir = KGlobal::dirs()->saveLocation( "appdata", "sessions/" + qApp->sessionId() + "/", true );

//     // FIXME: for some reason the config entries are not properly stored when using the default
//     //        KMainWindow session config. Since I was not able to find the bug I use another config object
//     // ----------------------------------------------------------
//     KConfig c( saveDir + "list", KConfig::SimpleConfig );
//     KConfigGroup grp( &c, "Saved Session" );
//     // ----------------------------------------------------------

    QList<K3b::Doc*> docs = k3bappcore->projectManager()->projects();
    grp.writeEntry( "Number of projects", docs.count() );

    int cnt = 1;
    Q_FOREACH( K3b::Doc* doc, docs ) {
        // the "name" of the project (or the original url if isSaved())
        grp.writePathEntry( QString("%1 url").arg(cnt), (doc)->URL().url() );

        // is the doc modified
        grp.writeEntry( QString("%1 modified").arg(cnt), (doc)->isModified() );

        // has the doc already been saved?
        grp.writeEntry( QString("%1 saved").arg(cnt), (doc)->isSaved() );

        // where does the session management save it? If it's not modified and saved this is
        // the same as the url
        KUrl saveUrl = (doc)->URL();
        if( !(doc)->isSaved() || (doc)->isModified() )
            saveUrl = KUrl( saveDir + QString::number(cnt) );
        grp.writePathEntry( QString("%1 saveurl").arg(cnt), saveUrl.url() );

        // finally save it
        k3bappcore->projectManager()->saveProject( doc, saveUrl );

        ++cnt;
    }

//    c.sync();
}


// FIXME:move this to K3b::ProjectManager
void K3b::MainWindow::readProperties( const KConfigGroup& grp )
{
    // FIXME: do not delete the files here. rather do it when the app is exited normally
    //        since that's when we can be sure we never need the session stuff again.

    // 1. read all projects from the config
    // 2. simply open all of themg
    // 3. reset the saved urls and the modified state
    // 4. delete "~/.kde/share/apps/k3b/sessions/" + KApp->sessionId()

    QString saveDir = KGlobal::dirs()->saveLocation( "appdata", "sessions/" + qApp->sessionId() + "/", true );

//     // FIXME: for some reason the config entries are not properly stored when using the default
//     //        KMainWindow session config. Since I was not able to find the bug I use another config object
//     // ----------------------------------------------------------
//     KConfig c( saveDir + "list"/*, true*/ );
//     KConfigGroup grp( &c, "Saved Session" );
//     // ----------------------------------------------------------

    int cnt = grp.readEntry( "Number of projects", 0 );
/*
  kDebug() << "(K3b::MainWindow::readProperties) number of projects from last session in " << saveDir << ": " << cnt << endl
  << "                                read from config group " << c->group() << endl;
*/
    for( int i = 1; i <= cnt; ++i ) {
        // in this case the constructor works since we saved as url()
        KUrl url = grp.readPathEntry( QString("%1 url").arg(i),QString() );

        bool modified = grp.readEntry( QString("%1 modified").arg(i),false );

        bool saved = grp.readEntry( QString("%1 saved").arg(i),false );

        KUrl saveUrl = grp.readPathEntry( QString("%1 saveurl").arg(i),QString() );

        // now load the project
        if( K3b::Doc* doc = k3bappcore->projectManager()->openProject( saveUrl ) ) {

            // reset the url
            doc->setURL( url );
            doc->setModified( modified );
            doc->setSaved( saved );
        }
        else
            kDebug() << "(K3b::MainWindow) could not open session saved doc " << url.path();

        // remove the temp file
        if( !saved || modified )
            QFile::remove( saveUrl.path() );
    }

    // and now remove the temp dir
    KIO::del( KUrl(saveDir), KIO::HideProgressInfo );
}


bool K3b::MainWindow::queryClose()
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

//     kDebug() << "(K3b::MainWindow::queryClose) jobs running.";
//     K3b::Job* job = k3bcore->runningJobs().getFirst();

//     // now search for the major job (to be on the safe side although for now no subjobs register with the k3bcore)
//     K3b::JobHandler* jh = job->jobHandler();
//     while( jh->isJob() ) {
//       job = static_cast<K3b::Job*>( jh );
//       jh = job->jobHandler();
//     }

//     kDebug() << "(K3b::MainWindow::queryClose) main job found: " << job->jobDescription();

//     // now job is the major job and jh should be a widget
//     QWidget* progressDialog = dynamic_cast<QWidget*>( jh );

//     kDebug() << "(K3b::MainWindow::queryClose) job active: " << job->active();

//     // now ask the user if he/she really wants to cancel this job
//     if( job->active() ) {
//       if( KMessageBox::questionYesNo( progressDialog ? progressDialog : this,
// 				      i18n("Do you really want to cancel?"),
// 				      i18n("Cancel") ) == KMessageBox::Yes ) {
// 	// cancel the job
// 	kDebug() << "(K3b::MainWindow::queryClose) canceling job.";
// 	job->cancel();

// 	// wait for the job to finish
// 	kDebug() << "(K3b::MainWindow::queryClose) waiting for job to finish.";
// 	K3b::SignalWaiter::waitForJob( job );

// 	// close the progress dialog
// 	if( progressDialog ) {
// 	  kDebug() << "(K3b::MainWindow::queryClose) closing progress dialog.";
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

// 	kDebug() << "(K3b::MainWindow::queryClose) job cleanup done.";
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

    while( K3b::View* view = activeView() ) {
        if( !canCloseDocument(view->doc()) )
            return false;
        closeProject(view->doc());
    }

    return true;
}


bool K3b::MainWindow::canCloseDocument( K3b::Doc* doc )
{
    if( !doc->isModified() )
        return true;

    if( !KConfigGroup( config(), "General Options" ).readEntry( "ask_for_saving_changes_on_exit", true ) )
        return true;

    switch ( KMessageBox::warningYesNoCancel( this,
                                              i18n("%1 has unsaved data.", doc->URL().fileName() ),
                                              i18n("Closing Project"),
                                              KStandardGuiItem::save(),
                                              KStandardGuiItem::dontSave() ) ) {
    case KMessageBox::Yes:
        if ( !fileSave( doc ) )
            return false;
    case KMessageBox::No:
        return true;
    default:
        return false;
    }
}

bool K3b::MainWindow::queryExit()
{
    // TODO: call this in K3b::Application somewhere
    saveOptions();
    return true;
}



/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////


void K3b::MainWindow::slotFileOpen()
{
    slotStatusMsg(i18n("Opening file..."));

    KUrl::List urls = KFileDialog::getOpenUrls( KUrl(":k3b-projects-folder"),
                                                i18n("*.k3b|K3b Projects"),
                                                this,
                                                i18n("Open Files") );
    for( KUrl::List::iterator it = urls.begin(); it != urls.end(); ++it ) {
        openDocument( *it );
        actionFileOpenRecent->addUrl( *it );
    }
}

void K3b::MainWindow::slotFileOpenRecent(const KUrl& url)
{
    slotStatusMsg(i18n("Opening file..."));

    openDocument(url);
}


void K3b::MainWindow::slotFileSaveAll()
{
    Q_FOREACH( K3b::Doc* doc, k3bappcore->projectManager()->projects() ) {
        fileSave( doc );
    }
}


void K3b::MainWindow::slotFileSave()
{
    if( K3b::Doc* doc = activeDoc() ) {
        fileSave( doc );
    }
}

bool K3b::MainWindow::fileSave( K3b::Doc* doc )
{
    slotStatusMsg(i18n("Saving file..."));

    if( doc == 0 ) {
        doc = activeDoc();
    }

    if( doc != 0 ) {
        if( !doc->isSaved() )
            return fileSaveAs( doc );
        else if( !k3bappcore->projectManager()->saveProject( doc, doc->URL()) )
            KMessageBox::error (this,i18n("Could not save the current document."), i18n("I/O Error"));
    }

    return false;
}


void K3b::MainWindow::slotFileSaveAs()
{
    if( K3b::Doc* doc = activeDoc() ) {
        fileSaveAs( doc );
    }
}


bool K3b::MainWindow::fileSaveAs( K3b::Doc* doc )
{
    slotStatusMsg(i18n("Saving file with a new filename..."));

    if( !doc ) {
        doc = activeDoc();
    }

    if( doc ) {
        // we do not use the static KFileDialog method here to be able to specify a filename suggestion
        KFileDialog dlg( KUrl(":k3b-projects-folder"), i18n("*.k3b|K3b Projects"), this);
        dlg.setCaption( i18n("Save As") );
        dlg.setOperationMode( KFileDialog::Saving );
        dlg.setSelection( doc->name() );
        dlg.exec();
        KUrl url = dlg.selectedUrl();

        if( url.isValid() ) {
            KRecentDocument::add( url );

            bool exists = KIO::NetAccess::exists( url, KIO::NetAccess::DestinationSide, 0 );
            if( !exists ||
                KMessageBox::warningContinueCancel( this, i18n("Do you want to overwrite %1?", url.prettyUrl() ),
                                                    i18n("File Exists"), KGuiItem(i18n("Overwrite")) )
                == KMessageBox::Continue ) {

                if( k3bappcore->projectManager()->saveProject( doc, url ) ) {
                    actionFileOpenRecent->addUrl(url);
                    return true;
                }
                else {
                    KMessageBox::error (this,i18n("Could not save the current document."), i18n("I/O Error"));
                }
            }
        }
    }

    return false;
}


void K3b::MainWindow::slotFileClose()
{
    slotStatusMsg(i18n("Closing file..."));
    if( K3b::View* pView = activeView() ) {
        if( pView ) {
            K3b::Doc* pDoc = pView->doc();

            if( canCloseDocument(pDoc) ) {
                closeProject(pDoc);
            }
        }
    }

    slotCurrentDocChanged();
}


void K3b::MainWindow::slotFileCloseAll()
{
    while( K3b::View* pView = activeView() ) {
        if( pView ) {
            K3b::Doc* pDoc = pView->doc();

            if( canCloseDocument(pDoc) )
                closeProject(pDoc);
            else
                break;
        }
    }

    slotCurrentDocChanged();
}


void K3b::MainWindow::closeProject( K3b::Doc* doc )
{
    // unplug the actions
    if( factory() ) {
        if( d->lastDoc == doc ) {
            factory()->removeClient( static_cast<K3b::View*>(d->lastDoc->view()) );
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


void K3b::MainWindow::slotFileQuit()
{
    close();
}


void K3b::MainWindow::slotViewStatusBar()
{
    //turn Statusbar on or off
    if(actionViewStatusBar->isChecked()) {
        statusBar()->show();
    }
    else {
        statusBar()->hide();
    }
}


void K3b::MainWindow::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
//   statusBar()->clear();
//   statusBar()->setItemText(text,1);

    statusBar()->showMessage( text, 2000 );
}


void K3b::MainWindow::slotSettingsConfigure()
{
    K3b::OptionDialog d( this );

    d.exec();

    // emit a changed signal every time since we do not know if the user selected
    // "apply" and "cancel" or "ok"
    emit configChanged( config() );
}


void K3b::MainWindow::showOptionDialog( K3b::OptionDialog::ConfigPage index )
{
    K3b::OptionDialog d( this);
    d.setCurrentPage( index );

    d.exec();

    // emit a changed signal every time since we do not know if the user selected
    // "apply" and "cancel" or "ok"
    emit configChanged( config() );
}


K3b::Doc* K3b::MainWindow::slotNewAudioDoc()
{
    slotStatusMsg(i18n("Creating new Audio CD Project."));

    K3b::Doc* doc = k3bappcore->projectManager()->createProject( K3b::Doc::AUDIO );

    return doc;
}

K3b::Doc* K3b::MainWindow::slotNewDataDoc()
{
    slotStatusMsg(i18n("Creating new Data CD Project."));

    K3b::Doc* doc = k3bappcore->projectManager()->createProject( K3b::Doc::DATA );

    return doc;
}


K3b::Doc* K3b::MainWindow::slotContinueMultisession()
{
    return K3b::DataMultisessionImportDialog::importSession( 0, this );
}


K3b::Doc* K3b::MainWindow::slotNewVideoDvdDoc()
{
    slotStatusMsg(i18n("Creating new Video DVD Project."));

    K3b::Doc* doc = k3bappcore->projectManager()->createProject( K3b::Doc::VIDEODVD );

    return doc;
}


K3b::Doc* K3b::MainWindow::slotNewMixedDoc()
{
    slotStatusMsg(i18n("Creating new Mixed Mode CD Project."));

    K3b::Doc* doc = k3bappcore->projectManager()->createProject( K3b::Doc::MIXED );

    return doc;
}

K3b::Doc* K3b::MainWindow::slotNewVcdDoc()
{
    slotStatusMsg(i18n("Creating new Video CD Project."));

    K3b::Doc* doc = k3bappcore->projectManager()->createProject( K3b::Doc::VCD );

    return doc;
}


K3b::Doc* K3b::MainWindow::slotNewMovixDoc()
{
    slotStatusMsg(i18n("Creating new eMovix Project."));

    K3b::Doc* doc = k3bappcore->projectManager()->createProject( K3b::Doc::MOVIX );

    return doc;
}


void K3b::MainWindow::slotCurrentDocChanged()
{
    // check the doctype
    K3b::View* v = activeView();
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
                factory()->removeClient( static_cast<K3b::View*>(d->lastDoc->view()) );
            factory()->addClient( v );
            d->lastDoc = v->doc();
        }
        else
            kDebug() << "(K3b::MainWindow) ERROR: could not get KXMLGUIFactory instance.";
    }
    else
        k3bappcore->projectManager()->setActive( 0L );

    if( k3bappcore->projectManager()->isEmpty() ) {
        slotStateChanged( "state_project_active", KXMLGUIClient::StateReverse );
    }
    else {
        slotStateChanged( "state_project_active", KXMLGUIClient::StateNoReverse );
    }

    // make sure the document header is shown (or not)
    slotViewDocumentHeader();
}


void K3b::MainWindow::slotEditToolbars()
{
    KConfigGroup grp( config(), "main_window_settings" );
    saveMainWindowSettings( grp );
    KEditToolBar dlg( factory() );
    connect( &dlg, SIGNAL(newToolbarConfig()), SLOT(slotNewToolBarConfig()) );
    dlg.exec();
}


void K3b::MainWindow::slotNewToolBarConfig()
{
    KConfigGroup grp(config(), "main_window_settings");
    applyMainWindowSettings(grp);
}


bool K3b::MainWindow::eject()
{
    KConfigGroup c( config(), "General Options" );
    return !c.readEntry( "No cd eject", false );
}


void K3b::MainWindow::slotErrorMessage(const QString& message)
{
    KMessageBox::error( this, message );
}


void K3b::MainWindow::slotWarningMessage(const QString& message)
{
    KMessageBox::sorry( this, message );
}


void K3b::MainWindow::slotWriteImage()
{
    K3b::ImageWritingDialog d( this );
    d.exec();
}


void K3b::MainWindow::slotWriteImage( const KUrl& url )
{
    K3b::ImageWritingDialog d( this );
    d.setImage( url );
    d.exec();
}


void K3b::MainWindow::slotProjectAddFiles()
{
    K3b::View* view = activeView();

    if( view ) {
        const QStringList files = KFileDialog::getOpenFileNames( KUrl(":k3b-project-add-files"),
                                                           i18n("*|All Files"),
                                                           this,
                                                           i18n("Select Files to Add to Project") );

        KUrl::List urls;
        for( QStringList::ConstIterator it = files.constBegin();
             it != files.constEnd(); it++ ) {
            KUrl url;
            url.setPath(*it);
            urls.append( url );
        }

        if( !urls.isEmpty() )
            view->addUrls( urls );
    }
    else
        KMessageBox::error( this, i18n("Please create a project before adding files"), i18n("No Active Project"));
}


void K3b::MainWindow::slotK3bSetup()
{
    QStringList args("kcmshell4 k3bsetup2 --lang " + KGlobal::locale()->language());
    if( !KProcess::startDetached( K3b::findExe("kdesu"), args ) )
        KMessageBox::error( 0, i18n("Could not find kdesu to run K3b::Setup with root privileges. "
                                    "Please run it manually as root.") );
}


void K3b::MainWindow::formatMedium( K3b::Device::Device* dev )
{
    K3b::MediaFormattingDialog d( this );
    d.setDevice( dev );
    d.exec();
}


void K3b::MainWindow::slotFormatMedium()
{
    formatMedium( 0 );
}


void K3b::MainWindow::mediaCopy( K3b::Device::Device* dev )
{
    K3b::MediaCopyDialog d( this );
    d.setReadingDevice( dev );
    d.exec();
}


void K3b::MainWindow::slotMediaCopy()
{
    mediaCopy( 0 );
}


// void K3b::MainWindow::slotVideoDvdCopy()
// {
//   K3b::VideoDvdCopyDialog d( this );
//   d.exec();
// }



void K3b::MainWindow::slotShowMenuBar()
{
    if( menuBar()->isVisible() )
        menuBar()->hide();
    else
        menuBar()->show();
}


void K3b::MainWindow::slotShowTips()
{
    KTipDialog::showTip( this, QString(), true );
}



void K3b::MainWindow::slotViewDocumentHeader()
{
    if( actionViewDocumentHeader->isChecked() &&
        !k3bappcore->projectManager()->isEmpty() ) {
        m_documentHeader->show();
    }
    else {
        m_documentHeader->hide();
    }
}


K3b::ExternalBinManager* K3b::MainWindow::externalBinManager() const
{
    return k3bcore->externalBinManager();
}


K3b::Device::DeviceManager* K3b::MainWindow::deviceManager() const
{
    return k3bcore->deviceManager();
}


void K3b::MainWindow::slotDataImportSession()
{
    if( activeView() ) {
        if( K3b::DataView* view = dynamic_cast<K3b::DataView*>(activeView()) ) {
            view->importSession();
        }
    }
}


void K3b::MainWindow::slotDataClearImportedSession()
{
    if( activeView() ) {
        if( K3b::DataView* view = dynamic_cast<K3b::DataView*>(activeView()) ) {
            view->clearImportedSession();
        }
    }
}


void K3b::MainWindow::slotEditBootImages()
{
    if( activeView() ) {
        if( K3b::DataView* view = dynamic_cast<K3b::DataView*>(activeView()) ) {
            view->editBootImages();
        }
    }
}


void K3b::MainWindow::slotCheckSystemTimed()
{
    // run the system check from the event queue so we do not
    // mess with the device state resetting throughout the app
    // when called from K3b::DeviceManager::changed
    QTimer::singleShot( 0, this, SLOT(slotCheckSystem()) );
}


void K3b::MainWindow::slotCheckSystem()
{
    K3b::SystemProblemDialog::checkSystem( this, K3b::SystemProblemDialog::NotifyOnlyErrors );
}


void K3b::MainWindow::slotManualCheckSystem()
{
    K3b::SystemProblemDialog::checkSystem( this, K3b::SystemProblemDialog::AlwaysNotify );
}


void K3b::MainWindow::addUrls( const KUrl::List& urls )
{
    if( K3b::View* view = activeView() ) {
        view->addUrls( urls );
    }
    else {
        // check if the files are all audio we can handle. If so create an audio project
        bool audio = true;
        QList<K3b::Plugin*> fl = k3bcore->pluginManager()->plugins( "AudioDecoder" );
        for( KUrl::List::const_iterator it = urls.begin(); it != urls.end(); ++it ) {
            const KUrl& url = *it;

            if( QFileInfo(url.path()).isDir() ) {
                audio = false;
                break;
            }

            bool a = false;
            Q_FOREACH( K3b::Plugin* plugin, fl ) {
                if( static_cast<K3b::AudioDecoderFactory*>( plugin )->canDecode( url ) ) {
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
            K3b::CueFileParser parser( urls.first().path() );
            if( parser.isValid() && parser.toc().contentType() == K3b::Device::AUDIO ) {
                audio = true;
            }
        }

        if( audio )
            static_cast<K3b::View*>(slotNewAudioDoc()->view())->addUrls( urls );
        else if( urls.count() > 1 || !isCdDvdImageAndIfSoOpenDialog( urls.first() ) )
            static_cast<K3b::View*>(slotNewDataDoc()->view())->addUrls( urls );
    }
}


void K3b::MainWindow::slotClearProject()
{
    K3b::Doc* doc = k3bappcore->projectManager()->activeDoc();
    if( doc ) {
        if( KMessageBox::warningContinueCancel( this,
                                                i18n("Clear Project"),
                                                i18n("Do you really want to clear the current project?"),
                                                KGuiItem(i18n("Clear Project")),
                                                KGuiItem(i18n("Clear")),
                                                QString("clear_current_project_dontAskAgain") ) == KMessageBox::Continue ) {
            doc->clear();
        }
    }

}


bool K3b::MainWindow::isCdDvdImageAndIfSoOpenDialog( const KUrl& url )
{
    K3b::Iso9660 iso( url.path() );
    if( iso.open() ) {
        iso.close();
        slotWriteImage( url );
        return true;
    }
    else
        return false;
}


void K3b::MainWindow::slotCddaRip()
{
    cddaRip( 0 );
}


void K3b::MainWindow::cddaRip( K3b::Device::Device* dev )
{
    if( !dev ||
        !(k3bappcore->mediaCache()->medium( dev ).content() & K3b::Medium::CONTENT_AUDIO ) )
        dev = K3b::MediaSelectionDialog::selectMedium( K3b::Device::MEDIA_CD_ALL,
                                                     K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE,
                                                     K3b::Medium::CONTENT_AUDIO,
                                                     this,
                                                     i18n("Audio CD Rip") );

    if( dev )
        m_dirView->showDevice( dev );
}


void K3b::MainWindow::videoDvdRip( K3b::Device::Device* dev )
{
    if( !dev ||
        !(k3bappcore->mediaCache()->medium( dev ).content() & K3b::Medium::CONTENT_VIDEO_DVD ) )
        dev = K3b::MediaSelectionDialog::selectMedium( K3b::Device::MEDIA_DVD_ALL,
                                                     K3b::Device::STATE_COMPLETE,
                                                     K3b::Medium::CONTENT_VIDEO_DVD,
                                                     this,
                                                     i18n("Video DVD Rip") );

    if( dev )
        m_dirView->showDevice( dev );
}


void K3b::MainWindow::slotVideoDvdRip()
{
    videoDvdRip( 0 );
}


void K3b::MainWindow::videoCdRip( K3b::Device::Device* dev )
{
    if( !dev ||
        !(k3bappcore->mediaCache()->medium( dev ).content() & K3b::Medium::CONTENT_VIDEO_CD ) )
        dev = K3b::MediaSelectionDialog::selectMedium( K3b::Device::MEDIA_CD_ALL,
                                                     K3b::Device::STATE_COMPLETE,
                                                     K3b::Medium::CONTENT_VIDEO_CD,
                                                     this,
                                                     i18n("Video CD Rip") );

    if( dev )
        m_dirView->showDevice( dev );
}


void K3b::MainWindow::slotVideoCdRip()
{
    videoCdRip( 0 );
}


void K3b::MainWindow::showDiskInfo( K3b::Device::Device* dev )
{
    m_dirView->showDiskInfo( dev );
}

#include "k3b.moc"

