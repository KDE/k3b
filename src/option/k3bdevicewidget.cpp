/*
 *
 * $Id$
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


#include "k3bdevicewidget.h"
#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include <k3blistview.h>

#include <kinputdialog.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kdialog.h>
#include <klocale.h>
#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kio/global.h>

#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qstring.h>
#include <qcolor.h>
#include <qlist.h>
#include <q3header.h>
#include <QGridLayout>
#include <QtGui/QGroupBox>


class K3b::DeviceWidget::PrivateTempDevice
{
public:
    PrivateTempDevice( K3b::Device::Device* d ) {
        device = d;
        writer = d->burner();
    }

    K3b::Device::Device* device;
    bool writer;
};



K3b::DeviceWidget::DeviceWidget( K3b::Device::DeviceManager* manager, QWidget *parent )
    : QWidget( parent ),
      m_deviceManager( manager )
{
    QGridLayout* frameLayout = new QGridLayout( this );
    frameLayout->setSpacing( KDialog::spacingHint() );
    frameLayout->setMargin( 0 );


    // buttons
    // ------------------------------------------------
    QGridLayout* refreshButtonGrid = new QGridLayout;
    refreshButtonGrid->setSpacing( KDialog::spacingHint() );
    refreshButtonGrid->setMargin(0);
    m_buttonRefreshDevices = new QPushButton( i18n( "Refresh" ), this );
    m_buttonRefreshDevices->setToolTip( i18n( "Rescan the devices" ) );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    refreshButtonGrid->addItem( spacer, 0, 0 );
    refreshButtonGrid->addWidget( m_buttonRefreshDevices, 0, 2 );
    // ------------------------------------------------


    // Devices Box
    // ------------------------------------------------
    QGroupBox* groupDevices = new QGroupBox( i18n( "CD/DVD Drives" ), this );
    QVBoxLayout* groupDevicesLayout = new QVBoxLayout( groupDevices );
    groupDevicesLayout->setSpacing( KDialog::spacingHint() );
    groupDevicesLayout->setMargin( KDialog::marginHint() );

    m_viewDevices = new K3b::ListView( groupDevices );
    m_viewDevices->addColumn( "V" );
    m_viewDevices->addColumn( "D" );
    m_viewDevices->setAllColumnsShowFocus( true );
    m_viewDevices->header()->hide();
    m_viewDevices->setSorting( -1 );
    m_viewDevices->setDoubleClickForEdit(false);
    m_viewDevices->setAlternateBackground( QColor() );
    m_viewDevices->setSelectionMode( Q3ListView::NoSelection );
    m_viewDevices->setFullWidth(true);

    groupDevicesLayout->addWidget( m_viewDevices );
    // ------------------------------------------------


    frameLayout->addWidget( groupDevices, 0, 0 );
    frameLayout->addLayout( refreshButtonGrid, 1, 0 );
    // ------------------------------------------------

    // connections
    // ------------------------------------------------
    //  connect( m_buttonRefreshDevices, SIGNAL(clicked()), this, SLOT(slotRefreshDevices()) );
    connect( m_buttonRefreshDevices, SIGNAL(clicked()), this, SIGNAL(refreshButtonClicked()) );
    connect( m_deviceManager, SIGNAL(changed()), this, SLOT(init()) );
    // ------------------------------------------------
}


K3b::DeviceWidget::~DeviceWidget()
{
    qDeleteAll( m_tempDevices );
}


void K3b::DeviceWidget::init()
{
    // fill the temporary lists
    qDeleteAll( m_tempDevices );
    m_tempDevices.clear();

    // add the reading devices
    Q_FOREACH( K3b::Device::Device* dev, m_deviceManager->allDevices() )
        m_tempDevices.append( new PrivateTempDevice( dev ) );

    updateDeviceListViews();
}


void K3b::DeviceWidget::updateDeviceListViews()
{
    QColor disabledTextColor = palette().color( QPalette::Disabled, QPalette::Text );

    m_viewDevices->clear();

    // create the parent view items
    // -----------------------------------------
    m_writerParentViewItem = new Q3ListViewItem( m_viewDevices, i18n("Writer Drives") );
    m_writerParentViewItem->setPixmap( 0, SmallIcon( "media-optical-recordable" ) );
    // spacer item
    (void)new Q3ListViewItem( m_viewDevices );
    m_readerParentViewItem = new Q3ListViewItem( m_viewDevices, i18n("Readonly Drives") );
    m_readerParentViewItem->setPixmap( 0, SmallIcon( "media-optical" ) );
    // -----------------------------------------

    QFont fBold( m_viewDevices->font() );
    fBold.setBold(true);
    QFont fItalic( m_viewDevices->font() );
    fItalic.setItalic(true);

    foreach( PrivateTempDevice* dev, m_tempDevices ) {
        // create the root device item
        K3b::ListViewItem* devRoot = new K3b::ListViewItem( (dev->writer ? m_writerParentViewItem : m_readerParentViewItem),
                                                        dev->device->vendor() + " " + dev->device->description() );
        devRoot->setFont( 0, fBold );

        // create the read-only info items
        K3b::ListViewItem* systemDeviceItem = new K3b::ListViewItem( devRoot, i18n("System device name:") );
        systemDeviceItem->setText( 1, dev->device->blockDeviceName() );
        systemDeviceItem->setForegroundColor( 1, disabledTextColor );

        K3b::ListViewItem* vendorItem = new K3b::ListViewItem( devRoot, systemDeviceItem,
                                                           i18n("Vendor:"),
                                                           dev->device->vendor() );
        vendorItem->setForegroundColor( 1, disabledTextColor );
        K3b::ListViewItem* modelItem = new K3b::ListViewItem( devRoot, vendorItem,
                                                          i18n("Description:"),
                                                          dev->device->description() );
        modelItem->setForegroundColor( 1, disabledTextColor );
        K3b::ListViewItem* versionItem = new K3b::ListViewItem( devRoot, modelItem,
                                                            i18n("Firmware:"),
                                                            dev->device->version() );
        versionItem->setForegroundColor( 1, disabledTextColor );


        // drive type
        // --------------------------------
        K3b::ListViewItem* typeItem = new K3b::ListViewItem( devRoot, versionItem,
                                                         i18n("Write Capabilities:"),
                                                         K3b::Device::mediaTypeString( dev->device->writeCapabilities(), true ) );
        typeItem->setForegroundColor( 1, disabledTextColor );
        typeItem = new K3b::ListViewItem( devRoot, typeItem,
                                        i18n("Read Capabilities:"),
                                        K3b::Device::mediaTypeString( dev->device->readCapabilities(), true ) );
        typeItem->setForegroundColor( 1, disabledTextColor );
        // --------------------------------


        // now add the reader (both interfaces) items
        if( dev->device->bufferSize() > 0 ) {
            typeItem = new K3b::ListViewItem( devRoot, typeItem,
                                            i18n("Buffer Size:"),
                                            KIO::convertSizeFromKiB(dev->device->bufferSize()) );
            typeItem->setForegroundColor( 1, disabledTextColor );
        }


        // now add the writer specific items
        if( dev->writer ) {
            typeItem = new K3b::ListViewItem( devRoot, typeItem,
                                            i18n("Supports Burnfree:"),
                                            dev->device->burnfree() ? i18n("yes") : i18n("no") );
            typeItem->setForegroundColor( 1, disabledTextColor );


            // and at last the write modes
            (new K3b::ListViewItem( devRoot,
                                  typeItem,
                                  i18n("Write modes:"),
                                  K3b::Device::writingModeString(dev->device->writingModes()) ))->setForegroundColor( 1, disabledTextColor );
        }

        devRoot->setOpen(true);
    }

    // create empty items
    if( m_writerParentViewItem->childCount() == 0 ) {
        K3b::ListViewItem* item = new K3b::ListViewItem( m_writerParentViewItem, i18n("none") );
        item->setFont( 0, fItalic );
    }
    if( m_readerParentViewItem->childCount() == 0 ) {
        K3b::ListViewItem* item = new K3b::ListViewItem( m_readerParentViewItem, i18n("none") );
        item->setFont( 0, fItalic );
    }

    m_writerParentViewItem->setOpen( true );
    m_readerParentViewItem->setOpen( true );
}

#include "k3bdevicewidget.moc"
