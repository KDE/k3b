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
#include "k3bcddb.h"
#include "k3bmediacache.h"
#include "k3bmodelutils.h"

#include <KComboBox>
#include <KLineEdit>
#include <KConfig>
#include <KStandardAction>
#include <KIconLoader>
#include <KLocalizedString>
#include <KNotification>
#include <KActionMenu>
#include <KMessageBox>
#include <KToolBarSpacerAction>
#include <KActionCollection>
#include <KToolBar>

#include <QDate>
#include <QDebug>
#include <QItemSelectionModel>
#include <QList>
#include <QFont>
#include <QKeyEvent>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QSpinBox>
#include <QTreeView>
#include <QVBoxLayout>

#include <KCddb/Genres>
#include <KCddb/Cdinfo>
#include <KCddb/Client>
#include "categories.h"

namespace mu = K3b::ModelUtils;

class K3b::AudioCdView::Private
{
public:
    KActionCollection* actionCollection;
    QMenu* popupMenu;

    AudioTrackModel* trackModel;
    QTreeView* trackView;
    KToolBar* toolBox;
    QLabel* labelLength;


    QLabel* busyInfoLabel;
};


K3b::AudioCdView::AudioCdView( QWidget* parent )
    : MediaContentsView( true,
                            Medium::ContentAudio,
                            Device::MEDIA_CD_ALL,
                            Device::STATE_INCOMPLETE | Device::STATE_COMPLETE,
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
    d->trackModel = new AudioTrackModel( this );
    d->trackView = new QTreeView( mainWidget() );
    d->trackView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    d->trackView->setModel( d->trackModel );
    d->trackView->setRootIsDecorated( false );
    d->trackView->setContextMenuPolicy( Qt::CustomContextMenu );
    d->trackView->setDragEnabled( true );
    d->trackView->installEventFilter( this );
    ViewColumnAdjuster* vca = new ViewColumnAdjuster( d->trackView );
    vca->addFixedColumn( AudioTrackModel::TrackNumberColumn );
    vca->addFixedColumn( AudioTrackModel::LengthColumn );
    vca->setColumnMargin( AudioTrackModel::LengthColumn, 10 );

    connect( d->trackView, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(slotContextMenu(QPoint)) );
    connect( d->trackView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(slotTrackSelectionChanged()) );
    connect( k3bcore->mediaCache(), SIGNAL(mediumCddbChanged(K3b::Device::Device*)),
             this, SLOT(slotCddbChanged(K3b::Device::Device*)) );

    mainGrid->addWidget( d->toolBox );
    mainGrid->addWidget( d->trackView );
    mainGrid->setSpacing( 0 );
    mainGrid->setContentsMargins( 0, 0, 0, 0 );

    initActions();

    // setup the toolbox
    d->toolBox->addAction( d->actionCollection->action( "start_rip" ) );
    d->toolBox->addSeparator();
    d->toolBox->addAction( d->actionCollection->action( "load_cd_info" ) );
    d->toolBox->addAction( d->actionCollection->action( "save_cddb_local" ) );
    d->toolBox->addAction( d->actionCollection->action( "edit_track_cddb" ) );
    d->toolBox->addAction( d->actionCollection->action( "edit_album_cddb" ) );
    d->toolBox->addSeparator();
    d->toolBox->addAction( d->actionCollection->action( "show_data_part" ) );
    d->toolBox->addAction( new KToolBarSpacerAction( d->toolBox ) );
    d->toolBox->addWidget( d->labelLength );

    slotTrackSelectionChanged();

    setLeftPixmap( Theme::MEDIA_LEFT );
    setRightPixmap( Theme::MEDIA_AUDIO );

    d->busyInfoLabel = new ThemedLabel( i18n("Searching for Artist information..."), this );
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

    loadCdInfo();

    actionCollection()->action( "show_data_part" )->setEnabled( medium().content() & Medium::ContentData );
    actionCollection()->action( "read_cd_text" )->setEnabled( !medium().cdText().isEmpty() );

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
            s += " (" + artist + ')';
        setTitle( s );
    }
    else {
        setTitle( i18n("Audio CD") );
    }
}


void K3b::AudioCdView::initActions()
{
    d->actionCollection = new KActionCollection( this );
    
    QAction* actionCheckTracks = new QAction( this );
    d->actionCollection->addAction( "check_tracks", actionCheckTracks );
    connect( actionCheckTracks, SIGNAL(triggered(bool)), this, SLOT(slotCheck()) );
    
    QAction* actionUncheckTracks = new QAction( this );
    d->actionCollection->addAction( "uncheck_tracks", actionUncheckTracks );
    connect( actionUncheckTracks, SIGNAL(triggered(bool)), this, SLOT(slotUncheck()) );
    
    QAction* actionEditTrackInfo = new QAction( QIcon::fromTheme( "document-properties" ), i18n("Edit Track Info..."), this );
    actionEditTrackInfo->setToolTip( i18n( "Edit current track information" ) );
    actionEditTrackInfo->setStatusTip( actionEditTrackInfo->toolTip() );
    d->actionCollection->addAction( "edit_track_cddb", actionEditTrackInfo );
    connect( actionEditTrackInfo, SIGNAL(triggered(bool)), this, SLOT(slotEditTrackCddb()) );
    
    QAction* actionEditAlbumInfo = new QAction( QIcon::fromTheme( "help-about" ), i18n("Edit Album Info..."), this );
    actionEditAlbumInfo->setToolTip( i18n( "Edit album information" ) );
    actionEditAlbumInfo->setStatusTip( actionEditAlbumInfo->toolTip() );
    d->actionCollection->addAction( "edit_album_cddb", actionEditAlbumInfo );
    connect( actionEditAlbumInfo, SIGNAL(triggered(bool)), this, SLOT(slotEditAlbumCddb()) );
    
    QAction* actionStartRip = new QAction( QIcon::fromTheme( "tools-rip-audio-cd" ), i18n("Start Ripping"), this );
    actionStartRip->setToolTip( i18n( "Start audio ripping process" ) );
    actionStartRip->setStatusTip( actionStartRip->toolTip() );
    d->actionCollection->addAction( "start_rip", actionStartRip );
    connect( actionStartRip, SIGNAL(triggered(bool)), this, SLOT(startRip()) );
    
    QAction* actionQueryCddb = new QAction( QIcon::fromTheme( "download" ), i18n("Query CD Database"), this );
    actionQueryCddb->setToolTip( i18n( "Look for information on CDDB" ) );
    actionQueryCddb->setStatusTip( actionQueryCddb->toolTip() );
    d->actionCollection->addAction( "query_cddb", actionQueryCddb );
    connect( actionQueryCddb, SIGNAL(triggered(bool)), this, SLOT(queryCddb()) );
    
    QAction* actionReadCdText = new QAction( QIcon::fromTheme( "media-optical" ), i18n("Read CD-Text"), this );
    actionReadCdText->setToolTip( i18n( "Read CD-Text information" ) );
    actionReadCdText->setStatusTip( actionReadCdText->toolTip() );
    d->actionCollection->addAction( "read_cd_text", actionReadCdText );
    connect( actionReadCdText, SIGNAL(triggered(bool)), this, SLOT(readCdText()) );
        
    KActionMenu* actionQueryInfo = new KActionMenu( QIcon::fromTheme( "view-refresh" ), i18n("Load CD Info"), this );
    actionQueryInfo->setToolTip( i18n( "Load track and album information" ) );
    actionQueryInfo->setStatusTip( actionQueryInfo->toolTip() );
    actionQueryInfo->addAction( actionQueryCddb );
    actionQueryInfo->addAction( actionReadCdText );
    d->actionCollection->addAction( "load_cd_info", actionQueryInfo );
    connect( actionQueryInfo, SIGNAL(triggered(bool)), this, SLOT(loadCdInfo()) );
    
    QAction* actionSaveCddb = new QAction( QIcon::fromTheme( "document-save" ), i18n("Save CD Info Locally"), this );
    actionSaveCddb->setToolTip( i18n( "Save track and album information to the local CDDB cache" ) );
    actionSaveCddb->setStatusTip( actionSaveCddb->toolTip() );
    d->actionCollection->addAction( "save_cddb_local", actionSaveCddb );
    connect( actionSaveCddb, SIGNAL(triggered(bool)), this, SLOT(slotSaveCddbLocally()) );
    
    QAction* actionShowDataPart = new QAction( QIcon::fromTheme( "media-optical-data" ), i18n("Show Data Part"), this );
    actionShowDataPart->setToolTip( i18n("Mounts the data part of CD") );
    actionShowDataPart->setStatusTip( actionShowDataPart->toolTip() );
    d->actionCollection->addAction( "show_data_part", actionShowDataPart );
    connect( actionShowDataPart, SIGNAL(triggered(bool)), this, SLOT(slotShowDataPart()) );
    
    QAction* actionSelectAll = KStandardAction::selectAll( d->trackView, SLOT(selectAll()), actionCollection() );

    // setup the popup menu
    d->popupMenu = new QMenu( this );
    d->popupMenu->addAction( actionCheckTracks );
    d->popupMenu->addAction( actionUncheckTracks );
    d->popupMenu->addSeparator();
    d->popupMenu->addAction( actionSelectAll );
    d->popupMenu->addSeparator();
    d->popupMenu->addAction( actionEditTrackInfo );
    d->popupMenu->addAction( actionEditAlbumInfo );
    d->popupMenu->addSeparator();
    d->popupMenu->addAction( d->actionCollection->action( "start_rip" ) );
    connect( d->popupMenu, SIGNAL(aboutToShow()), this, SLOT(slotContextMenuAboutToShow()) );
}


void K3b::AudioCdView::slotContextMenu( const QPoint& p )
{
    d->popupMenu->popup( d->trackView->mapToGlobal( p ) );
}


void K3b::AudioCdView::slotContextMenuAboutToShow()
{
    QAction *actionCheckTracks = actionCollection()->action("check_tracks");
    QAction *actionUncheckTracks = actionCollection()->action("uncheck_tracks");
    const QModelIndexList selectedRows = d->trackView->selectionModel()->selectedRows();

    if ( !selectedRows.empty() ) {
        const Qt::CheckState commonState = mu::commonCheckState( selectedRows );
        actionCheckTracks->setVisible( commonState != Qt::Checked );
        actionCheckTracks->setText( selectedRows.count() == 1 ? i18n("Check Track") : i18n("Check Tracks") );
        actionUncheckTracks->setVisible( commonState != Qt::Unchecked );
        actionUncheckTracks->setText( selectedRows.count() == 1 ? i18n("Uncheck Track") : i18n("Uncheck Tracks") );
    } else {
        actionCheckTracks->setVisible( false );
        actionUncheckTracks->setVisible( false );
    }
}


void K3b::AudioCdView::slotTrackSelectionChanged()
{
    actionCollection()->action("edit_track_cddb")->setEnabled( d->trackView->selectionModel()->hasSelection() );
}


void K3b::AudioCdView::startRip()
{
    QList<int> trackIndices = d->trackModel->checkedTrackIndices();
    if( trackIndices.count() == 0 ) {
        KMessageBox::error( this, i18n("Please select the tracks to rip."),
                            i18n("No Tracks Selected") );
    }
    else {
        AudioRippingDialog rip( medium(),
                                d->trackModel->cddbInfo(),
                                trackIndices,
                                this );
 
        rip.setDelayedInitialization( true );
        rip.exec();
    }
}


void K3b::AudioCdView::slotEditTrackCddb()
{
    const QModelIndexList selection = d->trackView->selectionModel()->selectedRows();
    if( !selection.isEmpty() ) {
        QDialog dialog( this );
        if( selection.size() > 1 )
            dialog.setWindowTitle( i18n( "Multiple Tracks" ) );
        else
            dialog.setWindowTitle( i18n( "CDDB Track %1", selection.first().data( AudioTrackModel::TrackNumberRole ).toInt() ) );
        dialog.setModal(true);

        KLineEdit* editTitle = new KLineEdit( mu::commonText( selection, AudioTrackModel::TitleRole ), this );
        KLineEdit* editArtist = new KLineEdit( mu::commonText( selection, AudioTrackModel::ArtistRole ), this );
        KLineEdit* editExtInfo = new KLineEdit( mu::commonText( selection, AudioTrackModel::CommentRole ), this );
        
        QFrame* line = new QFrame( this );
        line->setFrameShape( QFrame::HLine );
        line->setFrameShadow( QFrame::Sunken );

        QFormLayout* form = new QFormLayout(this);
        form->addRow( i18n("Title:"), editTitle );
        form->addRow( line );
        form->addRow( i18n("Artist:"), editArtist );
        form->addRow( i18n("Extra info:"), editExtInfo );
        form->setContentsMargins( 0, 0, 0, 0 );

        QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
        connect( buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()) );
        connect( buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()) );

        QVBoxLayout* dlgLayout = new QVBoxLayout( &dialog );
        dlgLayout->addLayout( form );
        dlgLayout->addWidget( buttonBox );

        editTitle->setFocus( Qt::TabFocusReason );

        // load album's artist by default if no artist is specified yet
        if ( editArtist->text().isEmpty() )
            editArtist->setText( d->trackModel->cddbInfo().get( KCDDB::Artist ).toString() );

        if( dialog.exec() == QDialog::Accepted ) {
            mu::setCommonText( d->trackModel, selection, editTitle->text(), AudioTrackModel::TitleRole );
            mu::setCommonText( d->trackModel, selection, editArtist->text(), AudioTrackModel::ArtistRole );
            mu::setCommonText( d->trackModel, selection, editExtInfo->text(), AudioTrackModel::CommentRole );
        }
    }
}


void K3b::AudioCdView::slotEditAlbumCddb()
{
    QDialog dialog( this);
    dialog.setWindowTitle(i18n("Album CDDB"));
    dialog.setModal(true);

    KLineEdit* editTitle = new KLineEdit( d->trackModel->cddbInfo().get( KCDDB::Title ).toString(), this );
    KLineEdit* editArtist = new KLineEdit( d->trackModel->cddbInfo().get( KCDDB::Artist ).toString(), this );
    KLineEdit* editExtInfo = new KLineEdit( d->trackModel->cddbInfo().get( KCDDB::Comment ).toString(), this );
    QSpinBox* spinYear = new QSpinBox( this );
    spinYear->setRange( 1, 9999 );
    if ( d->trackModel->cddbInfo().get( KCDDB::Year ).toInt() == 0 ) {
        spinYear->setValue( QDate::currentDate().year() ); // set the current year to default if no year specified yet
    } else {
        spinYear->setValue( d->trackModel->cddbInfo().get( KCDDB::Year ).toInt() );
    }
    QFrame* line = new QFrame( this );
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );
    KComboBox* comboGenre = new KComboBox( this );
    comboGenre->addItems( KCDDB::Genres().i18nList() );
    KComboBox* comboCat = new KComboBox( this );
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

    QFormLayout* form = new QFormLayout(this);
    form->addRow( i18n("Title:"), editTitle );
    form->addRow( i18n("Artist:"), editArtist );
    form->addRow( i18n("Extra info:"), editExtInfo );
    form->addRow( i18n("Genre:"), comboGenre );
    form->addRow( i18n("Year:"), spinYear );
    form->addRow( line );
    form->addRow( i18n("Category:"), comboCat );
    form->setContentsMargins( 0, 0, 0, 0 );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
    connect( buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()) );
    connect( buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()) );

    QVBoxLayout* dlgLayout = new QVBoxLayout( &dialog );
    dlgLayout->addLayout( form );
    dlgLayout->addWidget( buttonBox );

    editTitle->setFocus(Qt::TabFocusReason);

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


void K3b::AudioCdView::loadCdInfo()
{
    // cddb vs. cd-text
    // ----------------
    if( !medium().cddbInfo().isValid() ) {
        readCdText();
    } else {
        //slotCddbChanged( medium().device() );
        queryCddb();
    }
}


void K3b::AudioCdView::queryCddb()
{
    enableInteraction( false );
    k3bcore->mediaCache()->lookupCddb( medium().device() );
}


void K3b::AudioCdView::readCdText()
{
    if( !medium().cdText().isEmpty() ) {
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
}


bool K3b::AudioCdView::eventFilter( QObject* obj, QEvent* event )
{
    if( event->type() == QEvent::KeyPress ) {
        // Due to limitation of default implementation of QTreeView
        // checking items with Space key doesn't work for columns other than first.
        // Using below code a user can do that.
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>( event );
        if( keyEvent->key() == Qt::Key_Space ) {
            if( keyEvent->modifiers().testFlag( Qt::ControlModifier ) ) {
                QItemSelectionModel* selectionModel = d->trackView->selectionModel();
                QModelIndex current = d->trackView->currentIndex();
                selectionModel->select( current, QItemSelectionModel::Toggle | QItemSelectionModel::Rows );
            } else {
                slotToggle();
            }
            return true;
        }
    }
    return MediaContentsView::eventFilter( obj, event );
}


void K3b::AudioCdView::slotSaveCddbLocally()
{
    KCDDB::Client cddbClient;
    cddbClient.config().load();
    cddbClient.store( d->trackModel->cddbInfo(), CDDB::createTrackOffsetList( d->trackModel->medium().toc() ) );
}


void K3b::AudioCdView::slotCheck()
{
    foreach( const QModelIndex& index, d->trackView->selectionModel()->selectedRows() ) {
        d->trackModel->setData( index, Qt::Checked, Qt::CheckStateRole );
    }
}


void K3b::AudioCdView::slotUncheck()
{
    foreach( const QModelIndex& index, d->trackView->selectionModel()->selectedRows() ) {
        d->trackModel->setData( index, Qt::Unchecked, Qt::CheckStateRole );
    }
}


void K3b::AudioCdView::slotToggle()
{
    mu::toggleCommonCheckState( d->trackModel, d->trackView->selectionModel()->selectedRows() );
}


void K3b::AudioCdView::slotShowDataPart()
{
    k3bappcore->appDeviceManager()->mountDisk( medium().device() );
}


void K3b::AudioCdView::slotCddbChanged( K3b::Device::Device* dev )
{
    if ( medium().device() == dev ) {
        // we only use the manually edited cddb in the model
        d->trackModel->setCddbInfo( medium().cddbInfo() );
    }
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


