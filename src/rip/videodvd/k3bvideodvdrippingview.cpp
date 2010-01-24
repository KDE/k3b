/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bvideodvdrippingview.h"
#include "k3bvideodvd.h"
#include "k3bvideodvdrippingdialog.h"
#include "k3bvideodvdtitletranscodingjob.h"
#include "k3bvideodvdtitledelegate.h"
#include "k3bvideodvdtitlemodel.h"

#include "k3bthememanager.h"
#include "k3bglobals.h"
#include "k3blibdvdcss.h"
#include "k3bcore.h"
#include "k3bexternalbinmanager.h"
#include "k3bmediacache.h"
#include "k3bmedium.h"

#include <QCursor>
#include <QDesktopServices>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QTreeView>

#include <KAction>
#include <KActionCollection>
#include <KApplication>
#include <KConfig>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KToolBar>
#include <KToolBarSpacerAction>
#include <KUrlLabel>


class K3b::VideoDVDRippingView::Private
{
public:
    KActionCollection* actionCollection;
    KMenu* popupMenu;

    KToolBar* toolBox;
    QLabel* labelLength;
    VideoDVDTitleDelegate* delegate;
    VideoDVDTitleModel* model;
    QTreeView* view;

    VideoDVD::VideoDVD dvd;
};

K3b::VideoDVDRippingView::VideoDVDRippingView( QWidget* parent )
    : K3b::MediaContentsView( true,
                            K3b::Medium::ContentVideoDVD,
                            K3b::Device::MEDIA_DVD_ALL,
                            K3b::Device::STATE_INCOMPLETE|K3b::Device::STATE_COMPLETE,
                            parent ),
      d( new Private )
{
    // toolbox
    // ----------------------------------------------------------------------------------
    d->toolBox = new KToolBar( mainWidget() );
    
    KUrlLabel* showFilesLabel = new KUrlLabel( d->toolBox );
    showFilesLabel->setText( i18n("Show files") );
    showFilesLabel->setWhatsThis( i18n("Shows plain Video DVD vob files from the DVD "
                                       "(including decryption) for further processing with another application") );
    connect( showFilesLabel, SIGNAL(leftClickedUrl()), this, SLOT(slotShowFiles()) );
    
    d->labelLength = new QLabel( d->toolBox );
    d->labelLength->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    d->labelLength->setContentsMargins( 0, 0, style()->pixelMetric( QStyle::PM_LayoutRightMargin ), 0 );

    d->delegate = new VideoDVDTitleDelegate( this );
    d->model = new VideoDVDTitleModel( this );

    initActions();

    d->toolBox->addAction( actionCollection()->action("start_rip") );
    d->toolBox->addSeparator();
    d->toolBox->addWidget( showFilesLabel );
    d->toolBox->addAction( new KToolBarSpacerAction( d->toolBox ) );
    d->toolBox->addWidget( d->labelLength );

    // the title view
    // ----------------------------------------------------------------------------------
    d->view = new QTreeView( mainWidget() );
    d->view->setItemDelegate( d->delegate );
    d->view->setModel( d->model );
    d->view->setRootIsDecorated( false );
    d->view->header()->setResizeMode( QHeaderView::ResizeToContents );
    d->view->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( d->view, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(slotContextMenu(const QPoint&)) );

    // general layout
    // ----------------------------------------------------------------------------------
    QVBoxLayout* mainGrid = new QVBoxLayout( mainWidget() );
    mainGrid->addWidget( d->toolBox );
    mainGrid->addWidget( d->view );
    mainGrid->setMargin( 0 );
    mainGrid->setSpacing( 0 );

    setLeftPixmap( K3b::Theme::MEDIA_LEFT );
    setRightPixmap( K3b::Theme::MEDIA_VIDEO );
}


K3b::VideoDVDRippingView::~VideoDVDRippingView()
{
    delete d;
}


KActionCollection* K3b::VideoDVDRippingView::actionCollection() const
{
    return d->actionCollection;
}


void K3b::VideoDVDRippingView::slotStartRipping()
{
    QList<int> titles = d->model->selectedTitles();

    if( titles.isEmpty() ) {
        KMessageBox::error( this, i18n("Please select the titles to rip."),
                            i18n("No Titles Selected") );
    }
    else {
        K3b::VideoDVDRippingDialog dlg( d->dvd, titles, this );
        dlg.exec();
    }
}


void K3b::VideoDVDRippingView::slotContextMenu( const QPoint& pos )
{
    d->popupMenu->popup( d->view->viewport()->mapToGlobal( pos ) );
}


void K3b::VideoDVDRippingView::slotCheck()
{
    Q_FOREACH( const QModelIndex& index, d->view->selectionModel()->selectedRows() )
    {
        d->model->setData( index, Qt::Checked, Qt::CheckStateRole );
    }
}


void K3b::VideoDVDRippingView::slotUncheck()
{
    Q_FOREACH( const QModelIndex& index, d->view->selectionModel()->selectedRows() )
    {
        d->model->setData( index, Qt::Unchecked, Qt::CheckStateRole );
    }
}


void K3b::VideoDVDRippingView::slotShowFiles()
{
    QUrl url;
    url.setScheme( "videodvd" );
    if( d->dvd.valid() ) {
        url.setPath( "/" + d->dvd.volumeIdentifier() );
    }
    QDesktopServices::openUrl( url );
}


void K3b::VideoDVDRippingView::reloadMedium()
{
    //
    // For VideoDVD reading it is important that the DVD is not mounted
    //
    if( K3b::isMounted( device() ) && !K3b::unmount( device() ) ) {
        KMessageBox::error( this,
                            i18n("K3b was unable to unmount device '%1' containing medium '%2'. "
                                 "Video DVD ripping will not work if the device is mounted. "
                                 "Please unmount manually.",
                            device()->blockDeviceName(),
                            k3bcore->mediaCache()->medium( device() ).shortString() ),
                            i18n("Unmounting failed") );
    }

    //
    // K3b::VideoDVD::open does not necessarily fail on encrypted DVDs if dvdcss is not
    // available. Thus, we test the availability of libdvdcss here
    //
    if( device()->copyrightProtectionSystemType() == K3b::Device::COPYRIGHT_PROTECTION_CSS ) {
        K3b::LibDvdCss* css = K3b::LibDvdCss::create();
        if( !css ) {
            KMessageBox::error( this, i18n("<p>Unable to read Video DVD contents: Found encrypted Video DVD."
                                           "<p>Install <i>libdvdcss</i> to get Video DVD decryption support.") );
            return;
        }
        else
            delete css;
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if( d->dvd.open( device() ) ) {
        setTitle( medium().beautifiedVolumeId() + " (" + i18n("Video DVD") + ")" );
        d->labelLength->setText( i18np("%1 title", "%1 titles", d->dvd.numTitles() ) );
        d->model->setVideoDVD( d->dvd );
        QApplication::restoreOverrideCursor();

        bool transcodeUsable = true;

        if( !k3bcore ->externalBinManager() ->foundBin( "transcode" ) ) {
            KMessageBox::sorry( this,
                                i18n("K3b uses transcode to rip Video DVDs. "
                                     "Please make sure it is installed.") );
            transcodeUsable = false;
        }
        else {
            int vc = 0, ac = 0;
            for( int i = 0; i < K3b::VideoDVDTitleTranscodingJob::VIDEO_CODEC_NUM_ENTRIES; ++i )
                if( K3b::VideoDVDTitleTranscodingJob::transcodeBinaryHasSupportFor( (K3b::VideoDVDTitleTranscodingJob::VideoCodec)i ) )
                    ++vc;
            for( int i = 0; i < K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_NUM_ENTRIES; ++i )
                if( K3b::VideoDVDTitleTranscodingJob::transcodeBinaryHasSupportFor( (K3b::VideoDVDTitleTranscodingJob::AudioCodec)i ) )
                    ++ac;
            if( !ac || !vc ) {
                KMessageBox::sorry( this,
                                    i18n("<p>K3b uses transcode to rip Video DVDs. "
                                         "Your installation of transcode lacks support for any of the "
                                         "codecs supported by K3b."
                                         "<p>Please make sure it is installed properly.") );
                transcodeUsable = false;
            }
        }

        actionCollection()->action("start_rip")->setEnabled( transcodeUsable );
    }
    else {
        QApplication::restoreOverrideCursor();

        KMessageBox::error( this, i18n("Unable to read Video DVD contents.") );
    }
}


void K3b::VideoDVDRippingView::enableInteraction( bool enable )
{
    actionCollection()->action( "start_rip" )->setEnabled( enable );
}


void K3b::VideoDVDRippingView::activate( bool active )
{
    if( !active )
    {
        //
        // For now we do it the easy way: just stop the preview generation
        // once this view is no longer selected one
        //
        d->model->stopPreviewGen();
    }
    
    MediaContentsView::activate( active );
}


void K3b::VideoDVDRippingView::initActions()
{
    d->actionCollection = new KActionCollection( this );

    KAction* actionSelectAll = new KAction( this );
    actionSelectAll->setText( i18n("Check All") );
    connect( actionSelectAll, SIGNAL( triggered() ), d->model, SLOT( checkAll() ) );
    actionCollection()->addAction( "check_all", actionSelectAll );

    KAction* actionDeselectAll = new KAction( this );
    actionDeselectAll->setText( i18n("Uncheck All") );
    connect( actionDeselectAll, SIGNAL( triggered() ), d->model, SLOT( uncheckAll() ) );
    actionCollection()->addAction( "uncheck_all", actionSelectAll );

    KAction* actionSelect = new KAction( this );
    actionSelect->setText( i18n("Check Track") );
    connect( actionSelect, SIGNAL( triggered() ), this, SLOT( slotCheck() ) );
    actionCollection()->addAction( "select_track", actionSelect );

    KAction* actionDeselect = new KAction( this );
    actionDeselect->setText( i18n("Uncheck Track") );
    connect( actionDeselect, SIGNAL( triggered() ), this, SLOT( slotUncheck() ) );
    actionCollection()->addAction( "deselect_track", actionDeselect );

    KAction* actionStartRip = new KAction( this );
    actionStartRip->setText( i18n("Start Ripping") );
    actionStartRip->setIcon( KIcon( "tools-rip-video-dvd" ) );
    actionStartRip->setToolTip( i18n("Open the Video DVD ripping dialog") );
    actionStartRip->setWhatsThis( i18n("<p>Rips single titles from a video DVD "
                                       "into a compressed format such as XviD. Menu structures are completely ignored."
                                       "<p>If you intend to copy the plain Video DVD vob files from the DVD "
                                       "(including decryption) for further processing with another application, "
                                       "please use \"Show files\" button."
                                       "<p>If you intend to make a copy of the entire Video DVD including all menus "
                                       "and extras it is recommended to use the K3b Copy tool.") );
    connect( actionStartRip, SIGNAL( triggered() ), this, SLOT( slotStartRipping() ) );
    actionCollection()->addAction( "start_rip", actionStartRip );

    // setup the popup menu
    d->popupMenu = new KMenu( this );
    d->popupMenu->addAction( actionSelect );
    d->popupMenu->addAction( actionDeselect );
    d->popupMenu->addAction( actionSelectAll );
    d->popupMenu->addAction( actionDeselectAll );
    d->popupMenu->addSeparator();
    d->popupMenu->addAction( actionStartRip );
}


#include "k3bvideodvdrippingview.moc"
