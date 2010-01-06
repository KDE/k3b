/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bpassivepopup.h"
#include "k3btoc.h"
#include "k3bdiskinfo.h"
#include "k3bdevicehandler.h"
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
#include <KStandardAction>
#include <KStandardDirs>
#include <KToolBar>

#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayout>
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


K3b::AudioCdView::AudioCdView( QWidget* parent )
    : K3b::MediaContentsView( true,
                            K3b::Medium::ContentAudio,
                            K3b::Device::MEDIA_CD_ALL,
                            K3b::Device::STATE_INCOMPLETE|K3b::Device::STATE_COMPLETE,
                            parent )
{
    QGridLayout* mainGrid = new QGridLayout( mainWidget() );

    // toolbox
    // ----------------------------------------------------------------------------------
    QHBoxLayout* toolBoxLayout = new QHBoxLayout;
    m_toolBox = new KToolBar( mainWidget() );
    toolBoxLayout->addWidget( m_toolBox );
    toolBoxLayout->addStretch( 0 );
    m_labelLength = new QLabel( mainWidget() );
    m_labelLength->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    toolBoxLayout->addWidget( m_labelLength );
    toolBoxLayout->addSpacing( style()->pixelMetric( QStyle::PM_LayoutRightMargin ) );


    // the track view
    // ----------------------------------------------------------------------------------
    m_trackModel = new K3b::AudioTrackModel( this );
    m_trackView = new QTreeView( mainWidget() );
    m_trackView->setModel( m_trackModel );
    m_trackView->setRootIsDecorated( false );
    m_trackView->setContextMenuPolicy( Qt::CustomContextMenu );
    m_trackView->setDragEnabled( true );
    K3b::ViewColumnAdjuster* vca = new K3b::ViewColumnAdjuster( m_trackView );
    vca->setFixedColumns( QList<int>() << 0 << 3 );
    vca->setColumnMargin( K3b::AudioTrackModel::LengthColumn, 10 );

    connect( m_trackView, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(slotContextMenu(const QPoint&)) );
    connect( m_trackView->selectionModel(), SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
             this, SLOT(slotTrackSelectionChanged()) );

    mainGrid->addLayout( toolBoxLayout, 0, 0 );
    mainGrid->addWidget( m_trackView, 1, 0 );
    mainGrid->setSpacing( 0 );
    mainGrid->setMargin( 0 );

    initActions();
    slotTrackSelectionChanged();

    setLeftPixmap( K3b::Theme::MEDIA_LEFT );
    setRightPixmap( K3b::Theme::MEDIA_AUDIO );

    m_busyInfoLabel = new K3b::ThemedLabel( i18n("Searching for Artist information..."), this );
    m_busyInfoLabel->setFrameStyle( QFrame::Box|QFrame::Plain );
    m_busyInfoLabel->setMargin( 6 );
    m_busyInfoLabel->hide();
}


K3b::AudioCdView::~AudioCdView()
{
}


void K3b::AudioCdView::reloadMedium()
{
    m_trackModel->setMedium( medium() );

    // we only use the manually edited cddb in the model
    m_trackModel->setCddbInfo( medium().cddbInfo() );

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

        m_trackModel->setCddbInfo( cddbInfo );
    }

    // update the title
    // ----------------
    updateTitle();

    // update the length label
    // -----------------------
    m_labelLength->setText( i18np("1 track (%2)",
                                  "%1 tracks (%2)",
                                  medium().toc().count(),
                                  medium().toc().length().toString()) );

    slotTrackSelectionChanged();

    enableInteraction( true );
}


void K3b::AudioCdView::updateTitle()
{
    QString title = m_trackModel->cddbInfo().get( KCDDB::Title ).toString();
    QString artist = m_trackModel->cddbInfo().get( KCDDB::Artist ).toString();
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
    m_actionCollection = new KActionCollection( this );

    KAction* actionSelectAll = K3b::createAction(this, i18n("Check All"), 0, 0, m_trackModel,
                                                 SLOT(checkAll()), actionCollection(),
                                                 "check_all" );
    KAction* actionDeselectAll = K3b::createAction(this, i18n("Uncheck All"), 0, 0, m_trackModel,
                                                   SLOT(uncheckAll()), actionCollection(),
                                                   "uncheck_all" );
    KAction* actionSelect = K3b::createAction(this, i18n("Check Track"), 0, 0, this,
                                              SLOT(slotSelect()), actionCollection(),
                                              "select_track" );
    KAction* actionDeselect = K3b::createAction(this, i18n("Uncheck Track"), 0, 0, this,
                                                SLOT(slotDeselect()), actionCollection(),
                                                "deselect_track" );
    KAction* actionEditTrackCddbInfo = K3b::createAction(this, i18n("Edit Track CDDB Info"), "document-properties", 0, this,
                                                         SLOT(slotEditTrackCddb()), actionCollection(),
                                                         "edit_track_cddb" );
    KAction* actionEditAlbumCddbInfo = K3b::createAction(this, i18n("Edit Album CDDB Info"), "help-about", 0, this,
                                                         SLOT(slotEditAlbumCddb()), actionCollection(),
                                                         "edit_album_cddb" );

    KAction* actionStartRip = K3b::createAction(this, i18n("Start Ripping"), "tools-rip-audio-cd", 0, this,
                                                SLOT(startRip()), actionCollection(), "start_rip" );

    KAction* actionQueryCddb = K3b::createAction(this, i18n("Query CDDB"), "view-refresh", 0, this,
                                                 SLOT(queryCddb()), actionCollection(), "query_cddb" );

    KAction* actionSaveCddbLocally = K3b::createAction(this, i18n("Save CDDB Entry Locally"), "document-save", 0, this,
                                                       SLOT(slotSaveCddbLocally()), actionCollection(), "save_cddb_local" );

    // TODO: set the actions tooltips and whatsthis infos

    // setup the popup menu
    m_popupMenu = new KMenu( this );
    m_popupMenu->addAction( actionSelect );
    m_popupMenu->addAction( actionDeselect );
    m_popupMenu->addAction( actionSelectAll );
    m_popupMenu->addAction( actionDeselectAll );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( actionEditTrackCddbInfo );
    m_popupMenu->addAction( actionEditAlbumCddbInfo );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( actionStartRip );

    // setup the toolbox
    m_toolBox->addAction( actionStartRip );
    //m_toolBox->addSpacing();
    m_toolBox->addAction( actionQueryCddb );
    m_toolBox->addAction( actionSaveCddbLocally );
    m_toolBox->addAction( actionEditTrackCddbInfo );
    m_toolBox->addAction( actionEditAlbumCddbInfo );
}


void K3b::AudioCdView::slotContextMenu( const QPoint& p )
{
    m_popupMenu->popup( m_trackView->mapToGlobal( p ) );
}


void K3b::AudioCdView::slotTrackSelectionChanged()
{
    bool itemsSelected = !selectedTrackIndices( m_trackView ).isEmpty();
    actionCollection()->action("edit_track_cddb")->setEnabled( itemsSelected );
    actionCollection()->action("select_track")->setEnabled( itemsSelected );
    actionCollection()->action("deselect_track")->setEnabled( itemsSelected );
}


void K3b::AudioCdView::startRip()
{
    QList<int> trackIndices = m_trackModel->checkedTrackIndices();
    if( trackIndices.count() == 0 ) {
        KMessageBox::error( this, i18n("Please select the tracks to rip."),
                            i18n("No Tracks Selected") );
    }
    else {
        K3b::AudioRippingDialog rip( medium(),
                                   m_trackModel->cddbInfo(),
                                   trackIndices,
                                   this );
        rip.setDelayedInitialization( true );
        rip.exec();
    }
}


void K3b::AudioCdView::slotEditTrackCddb()
{
    QList<int> items = selectedTrackIndices( m_trackView );
    if( !items.isEmpty() ) {
        int trackIndex = items.first();

        KDialog d( this);
        d.setCaption(i18n("CDDB Track %1", trackIndex) );
        d.setButtons(KDialog::Ok|KDialog::Cancel);
        d.setDefaultButton(KDialog::Ok);
        d.setModal(true);
        QWidget* w = new QWidget( &d );

        KLineEdit* editTitle = new KLineEdit( m_trackModel->data( m_trackModel->index( trackIndex, 0 ), K3b::AudioTrackModel::TitleRole ).toString(), w );
        KLineEdit* editArtist = new KLineEdit( m_trackModel->data( m_trackModel->index( trackIndex, 0 ), K3b::AudioTrackModel::ArtistRole ).toString(), w );
        KLineEdit* editExtInfo = new KLineEdit( m_trackModel->data( m_trackModel->index( trackIndex, 0 ), K3b::AudioTrackModel::CommentRole ).toString(), w );
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

        d.setMainWidget(w);
        d.resize( qMax( qMax(d.sizeHint().height(), d.sizeHint().width()), 300), d.sizeHint().height() );

        if( d.exec() == QDialog::Accepted ) {
            m_trackModel->setData( m_trackModel->index( trackIndex, 0 ), editTitle->text(), K3b::AudioTrackModel::TitleRole );
            m_trackModel->setData( m_trackModel->index( trackIndex, 0 ), editArtist->text(), K3b::AudioTrackModel::ArtistRole );
            m_trackModel->setData( m_trackModel->index( trackIndex, 0 ), editExtInfo->text(), K3b::AudioTrackModel::CommentRole );
        }
    }
}


void K3b::AudioCdView::slotEditAlbumCddb()
{
    KDialog d( this);
    d.setCaption(i18n("Album Cddb"));
    d.setModal(true);
    d.setButtons(KDialog::Ok|KDialog::Cancel);
    d.setDefaultButton(KDialog::Ok);
    QWidget* w = new QWidget( &d );

    KLineEdit* editTitle = new KLineEdit( m_trackModel->cddbInfo().get( KCDDB::Title ).toString(), w );
    KLineEdit* editArtist = new KLineEdit( m_trackModel->cddbInfo().get( KCDDB::Artist ).toString(), w );
    KLineEdit* editExtInfo = new KLineEdit( m_trackModel->cddbInfo().get( KCDDB::Comment ).toString(), w );
    QSpinBox* spinYear = new QSpinBox( w );
    spinYear->setRange( 1, 9999 );
    spinYear->setValue( m_trackModel->cddbInfo().get( KCDDB::Year ).toInt() );
    QFrame* line = new QFrame( w );
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );
    KComboBox* comboGenre = new KComboBox( w );
    comboGenre->addItems( KCDDB::Genres().i18nList() );
    KComboBox* comboCat = new KComboBox( w );
    comboCat->addItems( KCDDB::Categories().i18nList() );

    QString genre = m_trackModel->cddbInfo().get( KCDDB::Genre ).toString();
    QString cat = m_trackModel->cddbInfo().get( KCDDB::Category ).toString();

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

    d.setMainWidget(w);
    d.resize( qMax( qMax(d.sizeHint().height(), d.sizeHint().width()), 300), d.sizeHint().height() );

    if( d.exec() == QDialog::Accepted ) {
        KCDDB::CDInfo cddbInfo = m_trackModel->cddbInfo();
        cddbInfo.set( KCDDB::Title, editTitle->text() );
        cddbInfo.set( KCDDB::Artist, editArtist->text() );
        cddbInfo.set( KCDDB::Comment, editExtInfo->text() );
        cddbInfo.set( KCDDB::Category, KCDDB::Categories().i18n2cddb( comboCat->currentText() ) );
        cddbInfo.set( KCDDB::Genre, KCDDB::Genres().i18n2cddb( comboGenre->currentText() ) );
        cddbInfo.set( KCDDB::Year, spinYear->value() );
        m_trackModel->setCddbInfo( cddbInfo );

        updateTitle();
    }
}


void K3b::AudioCdView::queryCddb()
{
    enableInteraction( false );
    k3bcore->mediaCache()->lookupCddb( medium().device() );
}


void K3b::AudioCdView::slotSaveCddbLocally()
{
    KCDDB::Client cddbClient;
    cddbClient.config().readConfig();
    cddbClient.store( m_trackModel->cddbInfo(), K3b::CDDB::createTrackOffsetList( m_trackModel->medium().toc() ) );
    K3b::PassivePopup::showPopup( i18n("Saved entry in category %1.",
                                     m_trackModel->cddbInfo().get( KCDDB::Category ).toString() ),
                                i18n("CDDB") );
}


void K3b::AudioCdView::slotSelect()
{
    foreach( int track, selectedTrackIndices( m_trackView ) ) {
        m_trackModel->setTrackChecked( track, true );
    }
}


void K3b::AudioCdView::slotDeselect()
{
    foreach( int track, selectedTrackIndices( m_trackView ) ) {
        m_trackModel->setTrackChecked( track, false );
    }
}


void K3b::AudioCdView::showBusyLabel( bool b )
{
    if( !b ) {
        actionCollection()->action( "start_rip" )->setEnabled( true );
        m_trackView->setEnabled( true );
        m_busyInfoLabel->hide();
    }
    else {
        // the themed label is a cut label, thus its size hint is
        // based on the cut text, we force it to be full
        m_busyInfoLabel->resize( width(), height() );
        m_busyInfoLabel->resize( m_busyInfoLabel->sizeHint() );
        int x = (width() - m_busyInfoLabel->width())/2;
        int y = (height() - m_busyInfoLabel->height())/2;
        QRect r( QPoint( x, y ), m_busyInfoLabel->size() );
        m_busyInfoLabel->setGeometry( r );
        m_busyInfoLabel->show();

        m_trackView->setEnabled( false );
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
