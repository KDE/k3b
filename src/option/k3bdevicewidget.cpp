/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2010 Michal Malek <michalm@jabster.pl>
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
#include "config-k3b.h"

#include <KConfig>
#include <KIcon>
#include <KLocale>
#include <KStandardDirs>
#include <kio/global.h>

#include <QColor>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLayout>
#include <QList>
#include <QPushButton>
#include <QString>
#include <QToolTip>
#include <QTreeWidget>
#include <QVariant>
#include <QVBoxLayout>



K3b::DeviceWidget::DeviceWidget( K3b::Device::DeviceManager* manager, QWidget *parent )
    : QWidget( parent ),
      m_deviceManager( manager ),
      m_writerParentViewItem( 0 ),
      m_readerParentViewItem( 0 )
{
    QVBoxLayout* frameLayout = new QVBoxLayout( this );
    frameLayout->setContentsMargins( 0, 0, 0, 0 );


    // buttons
    // ------------------------------------------------
    QPushButton* modifyPermissions = new QPushButton( i18n( "Modify Permissions..." ), this );
    QPushButton* buttonRefreshDevices = new QPushButton( i18n( "Refresh" ), this );
    buttonRefreshDevices->setToolTip( i18n( "Rescan the devices" ) );
    QHBoxLayout* refreshButtonGrid = new QHBoxLayout;
    refreshButtonGrid->setContentsMargins( 0, 0, 0, 0 );
    refreshButtonGrid->addStretch();
    refreshButtonGrid->addWidget( modifyPermissions );
    refreshButtonGrid->addWidget( buttonRefreshDevices );
    // ------------------------------------------------

#ifndef BUILD_K3BSETUP
    modifyPermissions->setVisible( false );
#endif

    // Devices Box
    // ------------------------------------------------
    QGroupBox* groupDevices = new QGroupBox( i18n( "CD/DVD/BD Drives" ), this );
    QVBoxLayout* groupDevicesLayout = new QVBoxLayout( groupDevices );

    m_viewDevices = new QTreeWidget( groupDevices );
    m_viewDevices->setAllColumnsShowFocus( true );
    m_viewDevices->setHeaderHidden( true );
    m_viewDevices->setColumnCount( 2 );
    m_viewDevices->setSelectionMode( QAbstractItemView::NoSelection );
    m_viewDevices->setItemsExpandable( false );
    m_viewDevices->setRootIsDecorated( false );
    m_viewDevices->header()->setResizeMode( 0, QHeaderView::ResizeToContents );
    m_viewDevices->setFocusPolicy( Qt::NoFocus );
    m_viewDevices->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

    groupDevicesLayout->addWidget( m_viewDevices );
    // ------------------------------------------------


    frameLayout->addWidget( groupDevices );
    frameLayout->addLayout( refreshButtonGrid );
    // ------------------------------------------------

    // connections
    // ------------------------------------------------
    connect( modifyPermissions, SIGNAL(clicked()), this, SIGNAL(modifyPermissionsButtonClicked()) );
    connect( buttonRefreshDevices, SIGNAL(clicked()), this, SIGNAL(refreshButtonClicked()) );
    connect( m_deviceManager, SIGNAL(changed()), this, SLOT(init()) );
    // ------------------------------------------------
}


K3b::DeviceWidget::~DeviceWidget()
{
}


void K3b::DeviceWidget::init()
{
    updateDeviceListViews();
}


void K3b::DeviceWidget::updateDeviceListViews()
{
    QColor disabledTextColor = palette().color( QPalette::Disabled, QPalette::Text );

    m_viewDevices->clear();

    // create the parent view items
    // -----------------------------------------
    m_writerParentViewItem = new QTreeWidgetItem( m_viewDevices, QStringList() << i18n("Writer Drives") );
    m_writerParentViewItem->setIcon( 0, KIcon( "media-optical-recordable" ) );
    // spacer item
    QTreeWidgetItem* spacer = new QTreeWidgetItem( m_viewDevices );
    spacer->setFlags( Qt::NoItemFlags );
    m_readerParentViewItem = new QTreeWidgetItem( m_viewDevices, QStringList() << i18n("Read-only Drives") );
    m_readerParentViewItem->setIcon( 0, KIcon( "media-optical" ) );
    // -----------------------------------------

    QFont fBold( m_viewDevices->font() );
    fBold.setBold(true);
    QFont fItalic( m_viewDevices->font() );
    fItalic.setItalic(true);

    foreach( Device::Device* dev, m_deviceManager->allDevices() ) {
        // create the root device item
        QTreeWidgetItem* devRoot = new QTreeWidgetItem( (dev->burner() ? m_writerParentViewItem : m_readerParentViewItem),
                                                        QStringList() << ( dev->vendor() + " " + dev->description() ) );
        devRoot->setFont( 0, fBold );

        // create the read-only info items
        QTreeWidgetItem* systemDeviceItem = new QTreeWidgetItem( devRoot, QStringList() << i18n("System device name:") );
        systemDeviceItem->setText( 1, dev->blockDeviceName() );
        systemDeviceItem->setForeground( 1, disabledTextColor );
        systemDeviceItem->setTextAlignment( 0, Qt::AlignRight );

        QTreeWidgetItem* vendorItem = new QTreeWidgetItem( devRoot, systemDeviceItem );
        vendorItem->setText( 0, i18n("Vendor:") );
        vendorItem->setText( 1, dev->vendor() );
        vendorItem->setForeground( 1, disabledTextColor );
        vendorItem->setTextAlignment( 0, Qt::AlignRight );

        QTreeWidgetItem* modelItem = new QTreeWidgetItem( devRoot, vendorItem );
        modelItem->setText( 0, i18n("Description:") );
        modelItem->setText( 1, dev->description() );
        modelItem->setForeground( 1, disabledTextColor );
        modelItem->setTextAlignment( 0, Qt::AlignRight );

        QTreeWidgetItem* versionItem = new QTreeWidgetItem( devRoot, modelItem );
        versionItem->setText( 0, i18n("Firmware:") );
        versionItem->setText( 1, dev->version() );
        versionItem->setForeground( 1, disabledTextColor );
        versionItem->setTextAlignment( 0, Qt::AlignRight );


        // drive type
        // --------------------------------
        QTreeWidgetItem* typeItem = new QTreeWidgetItem( devRoot, versionItem );
        typeItem->setText( 0, i18n("Write Capabilities:") );
        typeItem->setText( 1, K3b::Device::mediaTypeString( dev->writeCapabilities(), true ) );
        typeItem->setToolTip( 1, typeItem->text(1) );
        typeItem->setForeground( 1, disabledTextColor );
        typeItem->setTextAlignment( 0, Qt::AlignRight );

        typeItem = new QTreeWidgetItem( devRoot, typeItem );
        typeItem->setText( 0, i18n("Read Capabilities:") );
        typeItem->setText( 1, K3b::Device::mediaTypeString( dev->readCapabilities(), true ) );
        typeItem->setToolTip( 1, typeItem->text(1) );
        typeItem->setForeground( 1, disabledTextColor );
        typeItem->setTextAlignment( 0, Qt::AlignRight );
        // --------------------------------


        // now add the reader (both interfaces) items
        if( dev->bufferSize() > 0 ) {
            typeItem = new QTreeWidgetItem( devRoot, typeItem );
            typeItem->setText( 0, i18n("Buffer Size:") );
            typeItem->setText( 1, KIO::convertSizeFromKiB(dev->bufferSize()) );
            typeItem->setForeground( 1, disabledTextColor );
            typeItem->setTextAlignment( 0, Qt::AlignRight );
        }


        // now add the writer specific items
        if( dev->burner() ) {
            typeItem = new QTreeWidgetItem( devRoot, typeItem );
            typeItem->setText( 0, i18n("Supports Burnfree:") );
            typeItem->setText( 1, dev->burnfree() ? i18n("yes") : i18n("no") );
            typeItem->setForeground( 1, disabledTextColor );
            typeItem->setTextAlignment( 0, Qt::AlignRight );

            // and at last the write modes
            typeItem = new QTreeWidgetItem( devRoot, typeItem );
            typeItem->setText( 0, i18n("Write modes:") );
            typeItem->setText( 1, K3b::Device::writingModeString(dev->writingModes()) );
            typeItem->setToolTip( 1, typeItem->text(1) );
            typeItem->setForeground( 1, disabledTextColor );
            typeItem->setTextAlignment( 0, Qt::AlignRight );
        }
    }

    // create empty items
    if( m_writerParentViewItem->childCount() == 0 ) {
        QTreeWidgetItem* item = new QTreeWidgetItem( m_writerParentViewItem );
        item->setText( 0, i18n("none") );
        item->setFont( 0, fItalic );
    }
    if( m_readerParentViewItem->childCount() == 0 ) {
        QTreeWidgetItem* item = new QTreeWidgetItem( m_readerParentViewItem );
        item->setText( 0, i18n("none") );
        item->setFont( 0, fItalic );
    }

    m_viewDevices->expandAll();
}

#include "k3bdevicewidget.moc"
