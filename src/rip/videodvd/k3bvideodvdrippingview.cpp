/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
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
#include "k3bvideodvdrippingtitlelistview.h"
#include "k3bvideodvdrippingdialog.h"

#include "k3bvideodvd.h"
#include "k3bvideodvdtitletranscodingjob.h"
#include "k3bthememanager.h"
#include "k3bglobals.h"
#include "k3blibdvdcss.h"
#include "k3bcore.h"
#include "k3bexternalbinmanager.h"
#include "k3bmediacache.h"
#include "k3bmedium.h"

#include <qcursor.h>
#include <qlayout.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QHBoxLayout>
#include <QGridLayout>

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>
#include <kconfig.h>
#include <KMenu>
#include <KActionCollection>
#include <KToolBar>


K3b::VideoDVDRippingView::VideoDVDRippingView( QWidget* parent )
    : K3b::MediaContentsView( true,
                            K3b::Medium::ContentVideoDVD,
                            K3b::Device::MEDIA_DVD_ALL,
                            K3b::Device::STATE_INCOMPLETE|K3b::Device::STATE_COMPLETE,
                            parent )
{
    QGridLayout* mainGrid = new QGridLayout( mainWidget() );

    // toolbox
    // ----------------------------------------------------------------------------------
    QHBoxLayout* toolBoxLayout = new QHBoxLayout;
    m_toolBox = new KToolBar( mainWidget(), true, true );
    toolBoxLayout->addWidget( m_toolBox );
    QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
    toolBoxLayout->addItem( spacer );
    m_labelLength = new QLabel( mainWidget() );
    m_labelLength->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    toolBoxLayout->addWidget( m_labelLength );


    // the title view
    // ----------------------------------------------------------------------------------
    m_titleView = new K3b::VideoDVDRippingTitleListView( mainWidget() );

    connect( m_titleView, SIGNAL(contextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)),
             this, SLOT(slotContextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)) );

    // general layout
    // ----------------------------------------------------------------------------------
    mainGrid->addLayout( toolBoxLayout, 0, 0 );
    mainGrid->addWidget( m_titleView, 1, 0 );


    initActions();

    m_toolBox->addAction( actionCollection()->action("start_rip") );

    setLeftPixmap( K3b::Theme::MEDIA_LEFT );
    setRightPixmap( K3b::Theme::MEDIA_VIDEO );
}


K3b::VideoDVDRippingView::~VideoDVDRippingView()
{
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

    if( m_dvd.open( device() ) ) {
        setTitle( medium().beautifiedVolumeId() + " (" + i18n("Video DVD") + ")" );
        m_labelLength->setText( i18np("%1 title", "%1 titles", m_dvd.numTitles() ) );
        m_titleView->setVideoDVD( m_dvd );
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


void K3b::VideoDVDRippingView::slotStartRipping()
{
    QList<int> titles;
    int i = 1;
    for( Q3ListViewItemIterator it( m_titleView ); *it; ++it, ++i )
        if( static_cast<K3b::CheckListViewItem*>( *it )->isChecked() )
            titles.append( i );

    if( titles.isEmpty() ) {
        KMessageBox::error( this, i18n("Please select the titles to rip."),
                            i18n("No Titles Selected") );
    }
    else {
        K3b::VideoDVDRippingDialog dlg( m_dvd, titles, this );
        dlg.exec();
    }
}


void K3b::VideoDVDRippingView::slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& p )
{
    m_popupMenu->popup(p);
}


void K3b::VideoDVDRippingView::slotCheckAll()
{
    for( Q3ListViewItemIterator it( m_titleView ); it.current(); ++it )
        dynamic_cast<K3b::CheckListViewItem*>(it.current())->setChecked(true);
}


void K3b::VideoDVDRippingView::slotUncheckAll()
{
    for( Q3ListViewItemIterator it( m_titleView ); it.current(); ++it )
        dynamic_cast<K3b::CheckListViewItem*>(it.current())->setChecked(false);
}


void K3b::VideoDVDRippingView::slotCheck()
{
    QList<Q3ListViewItem*> items( m_titleView->selectedItems() );
    Q_FOREACH( Q3ListViewItem* item, items )
        dynamic_cast<K3b::CheckListViewItem*>(item)->setChecked(true);
}


void K3b::VideoDVDRippingView::slotUncheck()
{
    QList<Q3ListViewItem*> items( m_titleView->selectedItems() );
    Q_FOREACH( Q3ListViewItem* item, items )
        dynamic_cast<K3b::CheckListViewItem*>(item)->setChecked(false);
}


void K3b::VideoDVDRippingView::initActions()
{
    m_actionCollection = new KActionCollection( this );

    KAction* actionSelectAll = new KAction( this );
    actionSelectAll->setText( i18n("Check All") );
    connect( actionSelectAll, SIGNAL( triggered() ), this, SLOT( slotCheckAll() ) );
    actionCollection()->addAction( "check_all", actionSelectAll );

    KAction* actionDeselectAll = new KAction( this );
    actionDeselectAll->setText( i18n("Uncheck All") );
    connect( actionDeselectAll, SIGNAL( triggered() ), this, SLOT( slotUncheckAll() ) );
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
    actionStartRip->setIcon( KIcon( "system-run" ) );
    actionStartRip->setToolTip( i18n("Open the Video DVD ripping dialog") );
    connect( actionStartRip, SIGNAL( triggered() ), this, SLOT( slotStartRipping() ) );
    actionCollection()->addAction( "start_rip", actionStartRip );

    // setup the popup menu
    m_popupMenu = new KMenu( this );
    m_popupMenu->addAction( actionSelect );
    m_popupMenu->addAction( actionDeselect );
    m_popupMenu->addAction( actionSelectAll );
    m_popupMenu->addAction( actionDeselectAll );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( actionStartRip );
}


void K3b::VideoDVDRippingView::enableInteraction( bool enable )
{
    actionCollection()->action( "start_rip" )->setEnabled( enable );
}


#include "k3bvideodvdrippingview.moc"
