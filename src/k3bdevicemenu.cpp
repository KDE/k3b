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
#include "projects/k3bdatamultisessionimportdialog.h"
#include "misc/k3bmediaformattingdialog.h"
#include "misc/k3bmediacopydialog.h"

#include <k3bmedium.h>
#include <k3bdevice.h>
#include <k3bmediacache.h>
#include <k3bglobals.h>
#include <k3bdevicehandler.h>

#include <KAction>
#include <KLocale>
#include <KIcon>
#include <KMessageBox>
#include <KInputDialog>



class K3b::DeviceMenu::Private
{
public:
    Private( DeviceMenu* parent )
        : device( 0 ),
          q( parent ) {
    }

    K3bDevice::Device* device;

    void rebuildMenu();
    void _k_aboutToShow();

    void _k_diskInfo();
    void _k_unlockDevice();
    void _k_lockDevice();
    void _k_mountDisk();
    void _k_unmountDisk();
    void _k_ejectDisk();
    void _k_loadDisk();
    void _k_setReadSpeed();
    void _k_copy();
    void _k_format();
    void _k_ripAudio();
    void _k_ripVcd();
    void _k_ripVideoDVD();
    void _k_continueMultisession();

private:
    DeviceMenu* q;
};


void K3b::DeviceMenu::Private::rebuildMenu()
{
    // FIXME: connect the rest of the actions

    K3bMedium medium = k3bcore->mediaCache()->medium( device );

    q->clear();

    if ( medium.content() == K3bMedium::CONTENT_DATA &&
         medium.diskInfo().appendable() ) {
        QAction* actionContinueMultisession = new QAction( KIcon( "datacd" ), i18n("Continue Multisession Project"), q );
        q->connect( actionContinueMultisession, SIGNAL( triggered() ), SLOT( _k_continueMultisession() ) );
        q->addAction( actionContinueMultisession );
    }

    // non-empty cd/dvd/bd: copy
    if ( !medium.diskInfo().empty() ) {
        QAction* actionCopy = new QAction( KIcon( "cdcopy" ), i18n("Copy &Medium..."), q );
        actionCopy->setToolTip( i18n("Open the media copy dialog") );
        q->connect( actionCopy, SIGNAL( triggered() ), SLOT( _k_copy() ) );
        q->addAction( actionCopy );
    }

    // rewritable: erase/format
    if ( !medium.diskInfo().empty() &&
         medium.diskInfo().rewritable() &&
         medium.diskInfo().mediaType() != K3bDevice::MEDIA_DVD_PLUS_RW ) {
        QAction* actionFormat = new QAction( KIcon( "formatdvd" ), i18n("&Format/Erase rewritable disk..."), q );
        actionFormat->setToolTip( i18n("Open the rewritable disk formatting/erasing dialog") );
        q->connect( actionFormat, SIGNAL( triggered() ), SLOT( _k_format() ) );
        q->addAction( actionFormat );
    }

    // audio content: rip audio
    if ( medium.content() & K3bMedium::CONTENT_AUDIO ) {
        QAction* actionRipAudio = new QAction( KIcon( "cddarip" ), i18n("Rip Audio CD..."), q );
        q->connect( actionRipAudio, SIGNAL( triggered() ), SLOT( _k_ripAudio() ) );
        q->addAction( actionRipAudio );
    }

    // video dvd: videodvd rip
    if ( medium.content() & K3bMedium::CONTENT_VIDEO_DVD ) {
        QAction* actionRipVideoDVD = new QAction( KIcon( "videodvd" ), i18n("Rip Video DVD..."), q );
        q->connect( actionRipVideoDVD, SIGNAL( triggered() ), SLOT( _k_ripVideoDVD() ) );
        q->addAction( actionRipVideoDVD );
    }

    // video cd: vcd rip
    if ( medium.content() & K3bMedium::CONTENT_VIDEO_CD ) {
        QAction* actionRipVcd = new QAction( KIcon( "videocd"), i18n("Rip Video CD..."), q );
        q->connect( actionRipVcd, SIGNAL( triggered() ), SLOT( _k_ripVcd() ) );
        q->addAction( actionRipVcd );
    }

    q->addSeparator();

    // always: eject, load, diskinfo, setReadSpeed
    QAction* actionDiskInfo = new QAction( KIcon("document-properties"), i18n("Media &Info"), q );
    actionDiskInfo->setToolTip( i18n("Display generic medium information") );
    q->connect( actionDiskInfo, SIGNAL( triggered() ), SLOT(_k_diskInfo()) );
    q->addAction( actionDiskInfo );

    if ( K3b::isMounted( device ) ) {
        QAction* actionUnmount = new QAction( KIcon("media-optical"),  i18n("&Unmount"), q );
        actionUnmount->setToolTip( i18n("Unmount the medium") );
        q->connect( actionUnmount, SIGNAL( triggered() ), SLOT(_k_unmountDisk()) );
        q->addAction( actionUnmount );
    }
    else if ( medium.content() & K3bMedium::CONTENT_DATA ) {
        QAction* actionMount = new QAction( KIcon("media-optical"), i18n("&Mount"), q );
        actionMount->setToolTip( i18n("Mount the medium") );
        q->connect( actionMount, SIGNAL( triggered() ), SLOT(_k_mountDisk()) );
        q->addAction( actionMount );
    }

    QAction* actionLoad = new QAction( i18n("L&oad"), q );
    actionLoad->setToolTip( i18n("(Re)Load the medium") );
    q->connect( actionLoad, SIGNAL( triggered() ), SLOT(_k_loadDisk()) );
    q->addAction( actionLoad );

    QAction* actionEject = new QAction( i18n("&Eject"), q );
    actionEject->setToolTip( i18n("Eject the medium") );
    q->connect( actionEject, SIGNAL( triggered() ), SLOT(_k_ejectDisk()) );
    q->addAction( actionEject );

    if ( medium.content() & K3bMedium::CONTENT_DATA ) {
        QAction* actionSetReadSpeed = new QAction( i18n("Set Read Speed..."), q );
        actionSetReadSpeed->setToolTip( i18n("Force the drive's read speed") );
        q->connect( actionSetReadSpeed, SIGNAL( triggered() ), SLOT(_k_setReadSpeed()) );
        q->addAction( actionSetReadSpeed );
    }
}


void K3b::DeviceMenu::Private::_k_aboutToShow()
{
    rebuildMenu();
}


void K3b::DeviceMenu::Private::_k_diskInfo()
{
    k3bappcore->k3bMainWindow()->showDiskInfo( device );
}


void K3b::DeviceMenu::Private::_k_unlockDevice()
{
    if( device ) {
        K3bDevice::unblock( device );
    }
}


void K3b::DeviceMenu::Private::_k_lockDevice()
{
    if( device ) {
        K3bDevice::block( device );
    }
}


void K3b::DeviceMenu::Private::_k_mountDisk()
{
    if ( device ) {
        if( !K3b::isMounted( device ) )
            K3b::mount( device );
    }
}


void K3b::DeviceMenu::Private::_k_unmountDisk()
{
    if ( device ) {
        if( K3b::isMounted( device ) ) {
            K3b::unmount( device );
        }
    }
}


void K3b::DeviceMenu::Private::_k_ejectDisk()
{
    if ( device ) {
        K3b::eject( device );
    }
}


void K3b::DeviceMenu::Private::_k_loadDisk()
{
    if( device ) {
        K3bDevice::reload( device );
    }
}


void K3b::DeviceMenu::Private::_k_setReadSpeed()
{
    if( device ) {
        bool ok = false;
        int s = KInputDialog::getInteger( i18n("CD Read Speed"),
                                          i18n("<p>Please enter the preferred read speed for <b>%1</b>. "
                                               "This speed will be used for the currently mounted "
                                               "medium."
                                               "<p>This is especially useful to slow down the drive when "
                                               "watching movies which are read directly from the drive "
                                               "and the spinning noise is intrusive."
                                               "<p>Be aware that this has no influence on K3b since it will "
                                               "change the reading speed again when copying CDs or DVDs.",
                                               device->vendor() + " " + device->description()),
                                          12,
                                          1,
                                          device->maxReadSpeed(),
                                          1,
                                          10,
                                          &ok,
                                          0 );
        if( ok ) {
            if( !device->setSpeed( s*175, 0xFFFF ) ) {
                KMessageBox::error( 0, i18n("Setting the read speed failed.") );
            }
        }
    }
}


void K3b::DeviceMenu::Private::_k_copy()
{
    K3bMediaCopyDialog d( qApp->activeWindow() );
    d.setReadingDevice( device );
    d.exec();
}


void K3b::DeviceMenu::Private::_k_format()
{
    K3bMediaFormattingDialog d( qApp->activeWindow() );
    d.setDevice( device );
    d.exec();
}


void K3b::DeviceMenu::Private::_k_ripAudio()
{
    k3bappcore->k3bMainWindow()->cddaRip( device );
}


void K3b::DeviceMenu::Private::_k_ripVcd()
{
    k3bappcore->k3bMainWindow()->videoCdRip( device );
}


void K3b::DeviceMenu::Private::_k_ripVideoDVD()
{
    k3bappcore->k3bMainWindow()->videoDvdRip( device );
}


void K3b::DeviceMenu::Private::_k_continueMultisession()
{
    K3bDataMultisessionImportDialog::importSession( 0, qApp->activeWindow() );
}


K3b::DeviceMenu::DeviceMenu( QWidget* parent )
    : KMenu( parent ),
      d( new Private(this) )
{
    connect( this, SIGNAL( aboutToShow() ),
             this, SLOT( _k_aboutToShow() ) );
}


K3b::DeviceMenu::~DeviceMenu()
{
    delete d;
}


void K3b::DeviceMenu::setDevice( K3bDevice::Device* dev )
{
    d->device = dev;
}


K3bDevice::Device* K3b::DeviceMenu::device() const
{
    return d->device;
}

#include "k3bdevicemenu.moc"
