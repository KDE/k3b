/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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

#include <config-k3b.h>
#include "k3bdirview.h"
#include "k3b.h"
#include "k3bappdevicemanager.h"
#include "k3bapplication.h"
#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3bdiskinfoview.h"
#include "k3bexternalbinmanager.h"
#include "k3bfileview.h"
#include "k3bfiletreeview.h"
#include "k3bmediacache.h"
#include "k3bthememanager.h"
#include "rip/k3baudiocdview.h"
#include "rip/k3bvideocdview.h"
#ifdef ENABLE_DVD_RIPPING
#include "rip/videodvd/k3bvideodvdrippingview.h"
#endif

#include <KConfigGroup>
#include <KNotification>
#include <KLocalizedString>
#include <KMessageBox>

#include <QDir>
#include <QUrl>
#include <QString>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>

class K3b::DirView::Private
{
public:
    QStackedWidget* viewStack;

    AudioCdView* cdView;
    VideoCdView* videoView;
#ifdef ENABLE_DVD_RIPPING
    VideoDVDRippingView* movieView;
#endif
    FileView* fileView;
    DiskInfoView* infoView;

    QSplitter* mainSplitter;
    FileTreeView* fileTreeView;

    bool bViewDiskInfo;
    bool contextMediaInfoRequested;

    void setCurrentView( K3b::ContentsView* view );
};


void K3b::DirView::Private::setCurrentView( K3b::ContentsView* view )
{
    if( ContentsView* previous = qobject_cast<ContentsView*>( viewStack->currentWidget() ) ) {
        previous->activate( false );
    }

    viewStack->setCurrentWidget( view );
    view->activate( true );
}


K3b::DirView::DirView( K3b::FileTreeView* treeView, QWidget* parent )
    : QWidget(parent),
      d( new Private )
{
    d->fileTreeView = treeView;
    d->bViewDiskInfo = false;
    d->contextMediaInfoRequested = false;

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    if( !d->fileTreeView ) {
        d->mainSplitter = new QSplitter( this );
        d->fileTreeView = new K3b::FileTreeView( d->mainSplitter );
        d->viewStack    = new QStackedWidget( d->mainSplitter );
        layout->addWidget( d->mainSplitter );
    }
    else {
        d->viewStack    = new QStackedWidget( this );
        d->mainSplitter = 0;
        layout->addWidget( d->viewStack );
    }

    d->fileView     = new K3b::FileView( d->viewStack );
    d->viewStack->addWidget( d->fileView );
    d->cdView       = new K3b::AudioCdView( d->viewStack );
    d->viewStack->addWidget( d->cdView );
    d->videoView    = new K3b::VideoCdView( d->viewStack );
    d->viewStack->addWidget( d->videoView );
    d->infoView     = new K3b::DiskInfoView( d->viewStack );
    d->viewStack->addWidget( d->infoView );
#ifdef ENABLE_DVD_RIPPING
    d->movieView    = new K3b::VideoDVDRippingView( d->viewStack );
    d->viewStack->addWidget( d->movieView );
#endif

    d->setCurrentView( d->fileView );

//     d->fileTreeView->setCurrentDevice( k3bappcore->appDeviceManager()->currentDevice() );

    if( d->mainSplitter ) {
        // split
        QList<int> sizes = d->mainSplitter->sizes();
        int all = sizes[0] + sizes[1];
        sizes[1] = all*2/3;
        sizes[0] = all - sizes[1];
        d->mainSplitter->setSizes( sizes );
    }

    connect( d->fileTreeView, SIGNAL(activated(QUrl)),
             this, SLOT(slotDirActivated(QUrl)) );
    connect( d->fileTreeView, SIGNAL(activated(K3b::Device::Device*)),
             this, SLOT(showDevice(K3b::Device::Device*)) );
    connect( d->fileTreeView, SIGNAL(activated(K3b::Device::Device*)),
             this, SIGNAL(deviceSelected(K3b::Device::Device*)) );

    connect( d->fileView, SIGNAL(urlEntered(QUrl)), d->fileTreeView, SLOT(setSelectedUrl(QUrl)) );
    connect( d->fileView, SIGNAL(urlEntered(QUrl)), this, SIGNAL(urlEntered(QUrl)) );

    connect( k3bappcore->appDeviceManager(), SIGNAL(mountFinished(QString)),
             this, SLOT(slotMountFinished(QString)) );
    connect( k3bappcore->appDeviceManager(), SIGNAL(unmountFinished(bool)),
             this, SLOT(slotUnmountFinished(bool)) );
}

K3b::DirView::~DirView()
{
    delete d;
}


void K3b::DirView::showUrl( const QUrl& url )
{
    qDebug() << url;
    slotDirActivated( url );
}


void K3b::DirView::showDevice( K3b::Device::Device* dev )
{
    d->contextMediaInfoRequested = true;
    d->fileTreeView->setSelectedDevice( dev );
    showMediumInfo( k3bappcore->mediaCache()->medium( dev ) );
}


void K3b::DirView::showDiskInfo( K3b::Device::Device* dev )
{
    d->contextMediaInfoRequested = false;
    d->fileTreeView->setSelectedDevice( dev );
    showMediumInfo( k3bappcore->mediaCache()->medium( dev ) );
}


void K3b::DirView::showMediumInfo( const K3b::Medium& medium )
{
    if( !d->contextMediaInfoRequested ||
        medium.diskInfo().diskState() == K3b::Device::STATE_EMPTY ||
        medium.diskInfo().diskState() == K3b::Device::STATE_NO_MEDIA ) {

        // show cd info
        d->setCurrentView( d->infoView );
        d->infoView->reload( medium );
        return;
    }

#ifdef ENABLE_DVD_RIPPING
    else if( medium.content() & K3b::Medium::ContentVideoDVD ) {
        d->movieView->reload( medium );
        d->setCurrentView( d->movieView );
        return;
    }
#endif

    else if( medium.content() & K3b::Medium::ContentVideoCD ) {
        if( k3bcore ->externalBinManager() ->foundBin( "vcdxrip" ) ) {
            d->setCurrentView( d->videoView );
            d->videoView->reload( medium );
        }
        else {
            KMessageBox::sorry( this, i18n("K3b uses vcdxrip from the vcdimager package to rip Video CDs. "
                                           "Please make sure it is installed.") );
            k3bappcore->appDeviceManager()->mountDisk( medium.device() );
        }
    }

    else if( medium.content() & K3b::Medium::ContentAudio ) {
        d->setCurrentView( d->cdView );
        d->cdView->reload( medium );
    }

    else if( medium.content() & K3b::Medium::ContentData ) {
        k3bappcore->appDeviceManager()->mountDisk( medium.device() );
    }

    else {
        // show cd info
        d->setCurrentView( d->infoView );
        d->infoView->reload( medium );
    }

    d->contextMediaInfoRequested = false;
}


void K3b::DirView::slotMountFinished( const QString& mp )
{
    if( !mp.isEmpty() ) {
        slotDirActivated( QUrl::fromLocalFile(mp) );
        d->fileView->reload(); // HACK to get the contents shown... FIXME
    }
    else {
        d->setCurrentView( d->fileView );
        KNotification::event( "MountFailed",
                              i18n("Mount Failed"),
                              i18n("<p>K3b was unable to mount medium <b>%1</b> in device <em>%2 - %3</em>",
                                   k3bappcore->mediaCache()->medium( k3bappcore->appDeviceManager()->currentDevice() ).shortString(),
                                   k3bappcore->appDeviceManager()->currentDevice()->vendor(),
                                   k3bappcore->appDeviceManager()->currentDevice()->description() ) );
    }
}


void K3b::DirView::slotUnmountFinished( bool success )
{
    if( success ) {
        // TODO: check if the fileview is still displaying a folder from the medium
    }
    else {
        KNotification::event( "MountFailed",
                              i18n("Unmount Failed"),
                              i18n("<p>K3b was unable to unmount medium <b>%1</b> in device <em>%2 - %3</em>",
                                   k3bappcore->mediaCache()->medium( k3bappcore->appDeviceManager()->currentDevice() ).shortString(),
                                   k3bappcore->appDeviceManager()->currentDevice()->vendor(),
                                   k3bappcore->appDeviceManager()->currentDevice()->description() ) );
    }
}


void K3b::DirView::slotDirActivated( const QUrl& url )
{
    qDebug() << url;
    d->fileView->setUrl( url, true );
    d->setCurrentView( d->fileView );
}


void K3b::DirView::home()
{
    slotDirActivated( QUrl::fromLocalFile(QDir::homePath()) );
}


void K3b::DirView::saveConfig( KConfigGroup grp )
{
    d->fileView->saveConfig(grp);
}


void K3b::DirView::readConfig( const KConfigGroup &grp )
{
    d->fileView->readConfig(grp);
}


