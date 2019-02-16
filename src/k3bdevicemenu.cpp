/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdevicemenu.h"
#include "k3b.h"
#include "k3bapplication.h"
#include "k3bappdevicemanager.h"
#include "projects/k3bdatamultisessionimportdialog.h"
#include "misc/k3bmediaformattingdialog.h"
#include "misc/k3bmediacopydialog.h"

#include "k3bmedium.h"
#include "k3bdevice.h"
#include "k3bmediacache.h"
#include "k3bdevicehandler.h"

#include <KLocalizedString>
#include <KActionCollection>
#include <QIcon>
#include <QAction>


class K3b::DeviceMenu::Private
{
public:
    Private( DeviceMenu* parent );

    void _k_aboutToShow();

    void _k_copy();
    void _k_format();
    void _k_ripAudio();
    void _k_ripVcd();
    void _k_ripVideoDVD();
    void _k_continueMultisession();

private:
    DeviceMenu* q;
    QAction* actionContinueMultisession;
    QAction* actionCopy;
    QAction* actionFormat;
    QAction* actionRipAudio;
    QAction* actionRipVideoDVD;
    QAction* actionRipVcd;
};


K3b::DeviceMenu::Private::Private( DeviceMenu* parent )
:
    q( parent )
{
    KActionCollection* dmActions = k3bappcore->appDeviceManager()->actionCollection();
    q->addAction( dmActions->action( "device_diskinfo" ) );
    q->addSeparator();
    q->addAction( dmActions->action( "device_unmount" ) );
    q->addAction( dmActions->action( "device_mount" ) );
    q->addAction( dmActions->action( "device_eject" ) );
    q->addAction( dmActions->action( "device_load" ) );
    q->addSeparator();
    q->addAction( dmActions->action( "device_set_read_speed" ) );

    q->addSeparator();

    // FIXME: Should we move these actions to K3b::AppDeviceManager?
    actionContinueMultisession = q->addAction( QIcon::fromTheme( "media-optical-data" ), i18n("Continue Multisession Project"), q, SLOT(_k_continueMultisession()) );
    actionCopy = q->addAction( QIcon::fromTheme( "tools-media-optical-copy" ), i18n("Copy &Medium..."), q, SLOT(_k_copy()) );
    actionFormat = q->addAction( QIcon::fromTheme( "tools-media-optical-erase" ), i18n("&Format/Erase rewritable disk..."), q, SLOT(_k_format()) );
    actionRipAudio = q->addAction( QIcon::fromTheme( "tools-rip-audio-cd" ), i18n("Rip Audio CD..."), q, SLOT(_k_ripAudio()) );
    actionRipVideoDVD = q->addAction( QIcon::fromTheme( "tools-rip-video-dvd" ), i18n("Rip Video DVD..."), q, SLOT(_k_ripVideoDVD()) );
    actionRipVcd = q->addAction( QIcon::fromTheme( "tools-rip-video-cd"), i18n("Rip Video CD..."), q, SLOT(_k_ripVcd()) );

    actionCopy->setToolTip( i18n("Open the media copy dialog") );
    actionCopy->setStatusTip( actionCopy->toolTip() );
    actionFormat->setToolTip( i18n("Open the rewritable disk formatting/erasing dialog") );
    actionFormat->setStatusTip( actionFormat->toolTip() );
}


void K3b::DeviceMenu::Private::_k_aboutToShow()
{
    K3b::Medium medium = k3bcore->mediaCache()->medium( k3bappcore->appDeviceManager()->currentDevice() );

    actionContinueMultisession->setVisible(
        medium.content() == K3b::Medium::ContentData &&
        medium.diskInfo().appendable() );

    // non-empty cd/dvd/bd: copy
    actionCopy->setVisible( !medium.diskInfo().empty() );

    // rewritable: erase/format
    actionFormat->setVisible (
        !medium.diskInfo().empty() &&
        medium.diskInfo().rewritable() &&
        medium.diskInfo().mediaType() != K3b::Device::MEDIA_DVD_PLUS_RW );

    // audio content: rip audio
    actionRipAudio->setVisible( medium.content() & K3b::Medium::ContentAudio );

    // video dvd: videodvd rip
    actionRipVideoDVD->setVisible( medium.content() & K3b::Medium::ContentVideoDVD );

    // video cd: vcd rip
    actionRipVcd->setVisible( medium.content() & K3b::Medium::ContentVideoCD );
}


void K3b::DeviceMenu::Private::_k_copy()
{
    K3b::MediaCopyDialog d( qApp->activeWindow() );
    d.setReadingDevice( k3bappcore->appDeviceManager()->currentDevice() );
    d.exec();
}


void K3b::DeviceMenu::Private::_k_format()
{
    K3b::MediaFormattingDialog d( qApp->activeWindow() );
    d.setDevice( k3bappcore->appDeviceManager()->currentDevice() );
    d.exec();
}


void K3b::DeviceMenu::Private::_k_ripAudio()
{
    k3bappcore->k3bMainWindow()->cddaRip( k3bappcore->appDeviceManager()->currentDevice() );
}


void K3b::DeviceMenu::Private::_k_ripVcd()
{
    k3bappcore->k3bMainWindow()->videoCdRip( k3bappcore->appDeviceManager()->currentDevice() );
}


void K3b::DeviceMenu::Private::_k_ripVideoDVD()
{
    k3bappcore->k3bMainWindow()->videoDvdRip( k3bappcore->appDeviceManager()->currentDevice() );
}


void K3b::DeviceMenu::Private::_k_continueMultisession()
{
    K3b::DataMultisessionImportDialog::importSession( 0, qApp->activeWindow() );
}


K3b::DeviceMenu::DeviceMenu( QWidget* parent )
    : QMenu( parent ),
      d( new Private(this) )
{
    connect( this, SIGNAL(aboutToShow()),
             this, SLOT(_k_aboutToShow()) );
}


K3b::DeviceMenu::~DeviceMenu()
{
    delete d;
}

#include "moc_k3bdevicemenu.cpp"
