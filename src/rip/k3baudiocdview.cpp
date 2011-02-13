/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010-2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiocdview.h"
#include "k3baudiorippingdialog.h"
#include "k3baudiotrackmodel.h"
#include "k3bviewcolumnadjuster.h"

#include "k3bappdevicemanager.h"
#include "k3btoc.h"
#include "k3bdiskinfo.h"
#include "k3bdevicehandler.h"
#include "k3bmedium.h"
#include "k3bmsf.h"
#include "k3bstdguiitems.h"
#include "k3bapplication.h"
#include "k3bthememanager.h"
#include "k3baudiocdtrackdrag.h"
#include "k3bthemedlabel.h"
#include "k3baction.h"
#include "k3bcddb.h"
#include "k3bmediacache.h"

#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KComboBox>
#include <KConfig>
#include <KDebug>
#include <KDialog>
#include <KIconLoader>
#include <KLineEdit>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KNotification>
#include <KStandardAction>
#include <KStandardDirs>
#include <KToolBar>
#include <KToolBarSpacerAction>

#include <QFont>
#include <QVBoxLayout>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QSpinBox>
#include <QTreeView>

#include <libkcddb/genres.h>
#include <libkcddb/cdinfo.h>
#include <libkcddb/client.h>
//#include <libkcddb/categories.h>
#include "categories.h"


namespace {
    QList<int> selectedTrackIndices( QTreeView* view ) {
        QList<int> l;
        foreach( const QModelIndex& index, view->selectionModel()->selectedRows() ) {
            l.append( index.row() );
        }
        return l;
    }
}


class K3b::AudioCdView::Private
{
public:
    KActionCollection* actionCollection;
    KMenu* popupMenu;

    K3b::AudioTrackModel* trackModel;
    QTreeView* trackView;
    KToolBar* toolBox;
    QLabel* labelLength;


    QLabel* busyInfoLabel;
};


K3b::AudioCdView::AudioCdView( QWidget* parent )
    : K3b::MediaContentsView( true,
                            K3b::Medium::ContentAudio,
                            K3b::Device::MEDIA_CD_ALL,
                            K3b::Device::STATE_INCOMPLETE|K3b::Device::STATE_COMPLETE,
                            parent ),
      d( new Private )
{
    QVBoxLayout* mainGrid = new QVBoxLayout( mainWidget() );

    // toolbox
    // ----------------------------------------------------------------------------------
    d->toolBox = new KToolBar( mainWidget() );
    d->labelLength = new QLabel( d->toolBox );
    d->labelLength->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    d->labelLength->setContentsMargins( 0, 0, style()->pixelMetric( QStyle::PM_LayoutRightMargin ), 0 );

    // the track view
    // ----------------------------------------------------------------------------------
    d->trackModel = new K3b::AudioTrackModel( this );
    d->trackView = new QTreeView( mainWidget() );
    d->trackView->setModel( d->trackModel );
    d->trackView->setRootIsDecorated( false );
    d->trackView->setContextMenuPolicy( Qt::CustomContextMenu );
    d->trackView->setDragEnabled( true );
    d->trackView->installEventFilter( this );
    K3b::ViewColumnAdjuster* vca = new K3b::ViewColumnAdjuster( d->trackView );
    vca->addFixedColumn( AudioTrackModel::TrackNumberColumn );
    vca->addFixedColumn( AudioTrackModel::LengthColumn );
    vca->setColumnMargin( AudioTrackModel::LengthColumn, 10 );

    connect( d->trackView, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(slotContextMenu(const QPoint&)) );
    connect( d->trackView->selectionModel(), SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
             this, SLOT(slotTrackSelectionChanged()) );

    mainGrid->addWidget( d->toolBox );
    mainGrid->addWidget( d->trackView );
    mainGrid->setSpacing( 0 );
    mainGrid->setContentsMargins( 0, 0, 0, 0 );

    initActions();

    // setup the toolbox
    d->toolBox->addAction( d->actionCollection->action( "start_rip" ) );
    d->toolBox->addSeparator();
    d->toolBox->addAction( d->actionCollection->action( "query_cddb" ) );
    d->toolBox->addAction( d->actionCollection->action( "save_cddb_local" ) );
    d->toolBox->addAction( d->actionCollection->action( "edit_track_cddb" ) );
    d->toolBox->addAction( d->actionCollection->action( "edit_album_cddb" ) );
    d->toolBox->addAction( d->actionCollection->action( "show_data_part" ) );
    d->toolBox->addAction( new KToolBarSpacerAction( d->toolBox ) );
    d->toolBox->addWidget( d->labelLength );

    slotTrackSelectionChanged();

    setLeftPixmap( K3b::Theme::MEDIA_LEFT );
    setRightPixmap( K3b::Theme::MEDIA_AUDIO );

    d->busyInfoLabel = new K3b::ThemedLabel( i18n("Searching for Artist information..."), this );
    d->busyInfoLabel->setFrameStyle( QFrame::Box|QFrame::Plain );
    d->busyInfoLabel->setContentsMargins( 6, 6, 6, 6 );
    d->busyInfoLabel->hide();
}


K3b::AudioCdView::~AudioCdView()
{
    delete d;
}


KActionCollection* K3b::AudioCdView::actionCollection() const
{
    return d->actionCollection;
}


void K3b::AudioCdView::reloadMedium()
{
    d->trackModel->setMedium( medium() );

    // we only use the manually edited cddb in the model
    d->trackModel->setCddbInfo( medium().cddbInfo() );

    // cddb vs. cd-text
    // ----------------
    if( !medium().cddbInfo().isValid() ||
        ( !medium().cdText().isEmpty() &&
          KMessageBox::questionYesNo( this,
                                      i18n("Found Cd-Text (%1 - %2). Do you want to use it instead of CDDB (%3 - %4)?",
                                           medium().cdText().performer(),
                                           medium().cdText().title(),
                                           medium().cddbInfo().get( KCDDB::Artist ).toString(),
                                           medium().cddbInfo().get( KCDDB::Title ).toString() ),
                                      i18n("Found Cd-Text"),
                                      KGuiItem( i18n("Use CD-Text") ),
                                      KGuiItem( i18n("Use CDDB") ),
                                      "prefereCdTextOverCddb" ) == KMessageBox::Yes ) ) {
        // simulate a cddb entry with the cdtext data
        KCDDB::CDInfo cddbInfo;
        cddbInfo.set( KCDDB::Artist, medium().cdText().performer() );
        cddbInfo.set( KCDDB::Title, medium().cdText().title() );
        cddbInfo.set( KCDDB::Comment, medium().cdText().message() );

        for( int i = 0; i < medium().cdText().count(); ++i ) {
            cddbInfo.track( i ).set( KCDDB::Title, medium().cdText()[i].title() );
            cddbInfo.track( i ).set( KCDDB::Artist, medium().cdText()[i].performer() );
            cddbInfo.track( i ).set( KCDDB::Comment, medium().cdText()[i].message() );
        }

        d->trackModel->setCddbInfo( cddbInfo );
    }

    actionCollection()->action( "show_data_part" )->setEnabled( medium().content() & Medium::ContentData );

    // update the title
    // ----------------
    updateTitle();

    // update the length label
    // -----------------------
    d->labelLength->setText( i18np("1 track (%2)",
                                  "%1 tracks (%2)",
                                  medium().toc().count(),
                                  medium().toc().length().toString()) );

    slotTrackSelectionChanged();

    enableInteraction( true );
}


void K3b::AudioCdView::updateTitle()
{
    QString title = d->trackModel->cddbInfo().get( KCDDB::Title ).toString();
    QString artist = d->trackModel->cddbInfo().get( KCDDB::Artist ).toString();
    if( !title.isEmpty() ) {
        QString s( title );
        if( !artist.isEmpty() )
            s += " (" + artist + ")";
        setTitle( s );
    }
    else {
        setTitle( i18n("Audio CD") );
    }
}


void K3b::AudioCdView::initActions()
{
    d->actionCollection = new KActionCollection( this );

    K3b::createAction(this, i18n("Check All"), 0, 0, d->trackModel,
                      SLOT(checkAll()), actionCollection(), "check_all" );
    K3b::createAction(this, i18n("Uncheck All"), 0, 0, d->trackModel,
                      SLOT(uncheckAll()), actionCollection(), "uncheck_all" );
    K3b::createAction(this, i18n("Check Track"), 0, 0, this,
                      SLOT(slotSelect()), actionCollection(), "select_track" );
    K3b::createAction(this, i18n("Uncheck Track"), 0, 0, this,
                      SLOT(slotDeselect()), actionCollection(), "deselect_track" );
    K3b::createAction(this, i18n("Edit Track CDDB Info"), "document-properties", 0, this,
                      SLOT(slotEditTrackCddb()), actionCollection(), "edit_track_cddb" );
    K3b::createAction(this, i18n("Edit Album CDDB Info"), "help-about", 0, this,
                      SLOT(slotEditAlbumCddb()), actionCollection(), "edit_album_cddb" );
    K3b::createAction(this, i18n("Start Ripping"), "tools-rip-audio-cd", 0, this,
                      SLOT(startRip()), actionCollection(), "start_rip" );
    K3b::createAction(this, i18n("Query CDDB"), "view-refresh", 0, this,
                      SLOT(queryCddb()), actionCollection(), "query_cddb" );
    K3b::createAction(this, i18n("Save CDDB Entry Locally"), "document-save", 0, this,
                      SLOT(slotSaveCddbLocally()), actionCollection(), "save_cddb_local" );
    KAction* actionShowDataPart = K3b::createAction( this, i18n("Show Data Part"), "media-optical-data", 0, this,
                                                     SLOT(slotShowDataPart()), actionCollection(), "show_data_part" );
    actionShowDataPart->setToolTip( i18n("Mounts the data part of CD") );

    // TODO: set the actions tooltips and whatsthis infos

    // setup the popup menu
    d->popupMenu = new KMenu( this );
    d->popupMenu->addAction( d->actionCollection->action( "select_track" ) );
    d->popupMenu->addAction( d->actionCollection->action( "deselect_track" ) );
    d->popupMenu->addAction( d->actionCollection->action( "check_all" ) );
    d->popupMenu->addAction( d->actionCollection->action( "uncheck_all" ) );
    d->popupMenu->addSeparator();
    d->popupMenu->addAction( d->actionCollection->action( "edit_track_cddb" ) );
    d->popupMenu->addAction( d->actionCollection->action( "edit_album_cddb" ) );
    d->popupMenu->addSeparator();
    d->popupMenu->addAction( d->actionCollection->action( "start_rip" ) );
}


void K3b::AudioCdView::slotContextMenu( const QPoint& p )
{
    d->popupMenu->popup( d->trackView->mapToGlobal( p ) );
}


void K3b::AudioCdView::slotTrackSelectionChanged()
{
    bool itemsSelected = !selectedTrackIndices( d->trackView ).isEmpty();
    actionCollection()->action("edit_track_cddb")->setEnabled( itemsSelected );
    actionCollection()->action("select_track")->setEnabled( itemsSelected );
    actionCollection()->action("deselect_track")->setEnabled( itemsSelected );
}


void K3b::AudioCdView::startRip()
{
    QList<int> trackIndices = d->trackModel->checkedTrackIndices();
    if( trackIndices.count() == 0 ) {
        KMessageBox::error( this, i18n("Please select the tracks to rip."),
                            i18n("No Tracks Selected") );
    }
    else {
        K3b::AudioRippingDialog rip( medium(),
                                   d->trackModel->cddbInfo(),
                                   trackIndices,
                                   this );
        rip.setDelayedInitialization( true );
        rip.exec();
    }
}


void K3b::AudioCdView::slotEditTrackCddb()
{
    QList<int> items = selectedTrackIndices( d->trackView );
    if( !items.isEmpty() ) {
        int trackIndex = items.first();

        KDialog dialog( this);
        dialog.setCaption(i18n("CDDB Track %1", trackIndex) );
        dialog.setButtons(KDialog::Ok|KDialog::Cancel);
        dialog.setDefaultButton(KDialog::Ok);
        dialog.setModal(true);
        QWidget* w = new QWidget( &dialog );

        KLineEdit* editTitle = new KLineEdit( d->trackModel->data( d->trackModel->index( trackIndex, 0 ), K3b::AudioTrackModel::TitleRole ).toString(), w );
        KLineEdit* editArtist = new KLineEdit( d->trackModel->data( d->trackModel->index( trackIndex, 0 ), K3b::AudioTrackModel::ArtistRole ).toString(), w );
        KLineEdit* editExtInfo = new KLineEdit( d->trackModel->data( d->trackModel->index( trackIndex, 0 ), K3b::AudioTrackModel::CommentRole ).toString(), w );
        QFrame* line = new QFrame( w );
        line->setFrameShape( QFrame::HLine );
        line->setFrameShadow( QFrame::Sunken );

        QGridLayout* grid = new QGridLayout( w );

        grid->addWidget( new QLabel( i18n("Title:"), w ), 0, 0 );
        grid->addWidget( editTitle, 0, 1 );
        grid->addWidget( line, 1, 0, 1, 2 );
        grid->addWidget( new QLabel( i18n("Artist:"), w ), 2, 0 );
        grid->addWidget( editArtist, 2, 1 );
        grid->addWidget( new QLabel( i18n("Extra info:"), w ), 3, 0 );
        grid->addWidget( editExtInfo, 3, 1 );
        grid->setRowStretch( 4, 1 );

        dialog.setMainWidget(w);
        dialog.resize( qMax( qMax(dialog.sizeHint().height(), dialog.sizeHint().width()), 300), dialog.sizeHint().height() );

        if( dialog.exec() == QDialog::Accepted ) {
            d->trackModel->setData( d->trackModel->index( trackIndex, 0 ), editTitle->text(), K3b::AudioTrackModel::TitleRole );
            d->trackModel->setData( d->trackModel->index( trackIndex, 0 ), editArtist->text(), K3b::AudioTrackModel::ArtistRole );
            d->trackModel->setData( d->trackModel->index( trackIndex, 0 ), editExtInfo->text(), K3b::AudioTrackModel::CommentRole );
        }
    }
}


void K3b::AudioCdView::slotEditAlbumCddb()
{
    KDialog dialog( this);
    dialog.setCaption(i18n("Album Cddb"));
    dialog.setModal(true);
    dialog.setButtons(KDialog::Ok|KDialog::Cancel);
    dialog.setDefaultButton(KDialog::Ok);
    QWidget* w = new QWidget( &dialog );

    KLineEdit* editTitle = new KLineEdit( d->trackModel->cddbInfo().get( KCDDB::Title ).toString(), w );
    KLineEdit* editArtist = new KLineEdit( d->trackModel->cddbInfo().get( KCDDB::Artist ).toString(), w );
    KLineEdit* editExtInfo = new KLineEdit( d->trackModel->cddbInfo().get( KCDDB::Comment ).toString(), w );
    QSpinBox* spinYear = new QSpinBox( w );
    spinYear->setRange( 1, 9999 );
    spinYear->setValue( d->trackModel->cddbInfo().get( KCDDB::Year ).toInt() );
    QFrame* line = new QFrame( w );
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );
    KComboBox* comboGenre = new KComboBox( w );
    comboGenre->addItems( KCDDB::Genres().i18nList() );
    KComboBox* comboCat = new KComboBox( w );
    comboCat->addItems( KCDDB::Categories().i18nList() );

    QString genre = d->trackModel->cddbInfo().get( KCDDB::Genre ).toString();
    QString cat = d->trackModel->cddbInfo().get( KCDDB::Category ).toString();

    for( int i = 0; i < comboCat->count(); ++i ) {
        if( comboCat->itemText(i) == cat ) {
            comboCat->setCurrentIndex(i);
            break;
        }
    }
    for( int i = 0; i < comboGenre->count(); ++i ) {
        if( comboGenre->itemText(i) == genre ) {
            comboGenre->setCurrentIndex(i);
            break;
        }
    }

    QGridLayout* grid = new QGridLayout( w );

    grid->addWidget( new QLabel( i18n("Title:"), w ), 0, 0 );
    grid->addWidget( editTitle, 0, 1 );
    grid->addWidget( new QLabel( i18n("Artist:"), w ), 1, 0 );
    grid->addWidget( editArtist, 1, 1 );
    grid->addWidget( new QLabel( i18n("Extra info:"), w ), 2, 0 );
    grid->addWidget( editExtInfo, 2, 1 );
    grid->addWidget( new QLabel( i18n("Genre:"), w ), 3, 0 );
    grid->addWidget( comboGenre, 3, 1 );
    grid->addWidget( new QLabel( i18n("Year:"), w ), 4, 0 );
    grid->addWidget( spinYear, 4, 1 );
    grid->addWidget( line, 5, 0, 1, 2 );
    grid->addWidget( new QLabel( i18n("Category:"), w ), 6, 0 );
    grid->addWidget( comboCat, 6, 1 );
    grid->setRowStretch( 7, 1 );

    dialog.setMainWidget(w);
    dialog.resize( qMax( qMax(dialog.sizeHint().height(), dialog.sizeHint().width()), 300), dialog.sizeHint().height() );

    if( dialog.exec() == QDialog::Accepted ) {
        KCDDB::CDInfo cddbInfo = d->trackModel->cddbInfo();
        cddbInfo.set( KCDDB::Title, editTitle->text() );
        cddbInfo.set( KCDDB::Artist, editArtist->text() );
        cddbInfo.set( KCDDB::Comment, editExtInfo->text() );
        cddbInfo.set( KCDDB::Category, KCDDB::Categories().i18n2cddb( comboCat->currentText() ) );
        cddbInfo.set( KCDDB::Genre, KCDDB::Genres().i18n2cddb( comboGenre->currentText() ) );
        cddbInfo.set( KCDDB::Year, spinYear->value() );
        d->trackModel->setCddbInfo( cddbInfo );

        updateTitle();
    }
}


void K3b::AudioCdView::queryCddb()
{
    enableInteraction( false );
    k3bcore->mediaCache()->lookupCddb( medium().device() );
}


bool K3b::AudioCdView::eventFilter( QObject* obj, QEvent* event )
{
    if( event->type() == QEvent::KeyPress ) {
        // Due to limitation of default implementation of QTreeView
        // checking items with Space key doesn't work for columns other than first.
        // Using below code a user can do that.
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>( event );
        if( keyEvent->key() == Qt::Key_Space ) {
            foreach( int track, selectedTrackIndices( d->trackView ) ) {
                d->trackModel->setTrackChecked( track, !d->trackModel->trackChecked( track ) );
            }
            return true;
        }
    }
    return MediaContentsView::eventFilter( obj, event );
}


void K3b::AudioCdView::slotSaveCddbLocally()
{
    KCDDB::Client cddbClient;
    cddbClient.config().readConfig();
    cddbClient.store( d->trackModel->cddbInfo(), K3b::CDDB::createTrackOffsetList( d->trackModel->medium().toc() ) );
    KNotification::event( KNotification::Notification,
                          i18n( "CDDB" ),
                          i18n( "Saved entry in category %1.",
                                d->trackModel->cddbInfo().get( KCDDB::Category ).toString() ) );
}


void K3b::AudioCdView::slotSelect()
{
    foreach( int track, selectedTrackIndices( d->trackView ) ) {
        d->trackModel->setTrackChecked( track, true );
    }
}


void K3b::AudioCdView::slotDeselect()
{
    foreach( int track, selectedTrackIndices( d->trackView ) ) {
        d->trackModel->setTrackChecked( track, false );
    }
}


void K3b::AudioCdView::slotShowDataPart()
{
    k3bappcore->appDeviceManager()->mountDisk( medium().device() );
}


void K3b::AudioCdView::showBusyLabel( bool b )
{
    if( !b ) {
        actionCollection()->action( "start_rip" )->setEnabled( true );
        d->trackView->setEnabled( true );
        d->busyInfoLabel->hide();
    }
    else {
        // the themed label is a cut label, thus its size hint is
        // based on the cut text, we force it to be full
        d->busyInfoLabel->resize( width(), height() );
        d->busyInfoLabel->resize( d->busyInfoLabel->sizeHint() );
        int x = (width() - d->busyInfoLabel->width())/2;
        int y = (height() - d->busyInfoLabel->height())/2;
        QRect r( QPoint( x, y ), d->busyInfoLabel->size() );
        d->busyInfoLabel->setGeometry( r );
        d->busyInfoLabel->show();

        d->trackView->setEnabled( false );
        enableInteraction( false );
    }
}


void K3b::AudioCdView::enableInteraction( bool b )
{
    // we leave the track view enabled in default disabled mode
    // since drag'n'drop to audio projects does not need an inserted CD
    actionCollection()->action( "start_rip" )->setEnabled( b );
    if( b )
        showBusyLabel( false );
}

#include "k3baudiocdview.moc"
