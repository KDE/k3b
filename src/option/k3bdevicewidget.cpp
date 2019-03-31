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


#include "k3bdevicewidget.h"
#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"

#include <KAuth>
#include <KConfig>
#include <KLocalizedString>
#include <KIO/Global>
#include <KMessageWidget>

#include <QFileInfo>
#include <QColor>
#include <QIcon>
#include <QAction>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLayout>
#include <QList>
#include <QPushButton>
#include <QString>
#include <QToolTip>
#include <QTreeWidget>
#include <QVariant>
#include <QVBoxLayout>

#include <grp.h>
#include <unistd.h>
#include <pwd.h>


K3b::DeviceWidget::DeviceWidget( K3b::Device::DeviceManager* manager, QWidget *parent )
    : QWidget( parent ),
      m_deviceManager( manager ),
      m_writerParentViewItem( 0 ),
      m_readerParentViewItem( 0 )
{
    // message widget
    m_messageWidget = new KMessageWidget( this );
    m_messageWidget->hide();
    m_messageWidget->setWordWrap( true );
    m_addToGroupAction = new QAction( QIcon::fromTheme("dialog-password"), QString(), this );

    // buttons
    // ------------------------------------------------
    QPushButton* buttonRefreshDevices = new QPushButton( QIcon::fromTheme( "view-refresh" ), i18n( "Refresh" ), this );
    buttonRefreshDevices->setToolTip( i18n( "Rescan the devices" ) );
    QHBoxLayout* refreshButtonGrid = new QHBoxLayout;
    refreshButtonGrid->setContentsMargins( 0, 0, 0, 0 );
    refreshButtonGrid->addStretch();
    refreshButtonGrid->addWidget( buttonRefreshDevices );
    // ------------------------------------------------

    // Devices view
    // ------------------------------------------------
    m_viewDevices = new QTreeWidget( this );
    m_viewDevices->setAllColumnsShowFocus( true );
    m_viewDevices->setHeaderHidden( true );
    m_viewDevices->setColumnCount( 2 );
    m_viewDevices->setSelectionMode( QAbstractItemView::NoSelection );
    m_viewDevices->setItemsExpandable( false );
    m_viewDevices->setRootIsDecorated( false );
    m_viewDevices->header()->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
    m_viewDevices->setFocusPolicy( Qt::NoFocus );
    m_viewDevices->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    // ------------------------------------------------

    QVBoxLayout* frameLayout = new QVBoxLayout( this );
    frameLayout->setContentsMargins( 0, 0, 0, 0 );
    frameLayout->addWidget( m_messageWidget, 0 );
    frameLayout->addWidget( m_viewDevices, 1 );
    frameLayout->addLayout( refreshButtonGrid, 0 );
    // ------------------------------------------------

    // connections
    // ------------------------------------------------
    connect( buttonRefreshDevices, SIGNAL(clicked()), SIGNAL(refreshButtonClicked()) );
    connect( m_deviceManager, SIGNAL(changed()), SLOT(init()) );
    connect( m_addToGroupAction, SIGNAL(triggered(bool)), SLOT(addUserToGroup()) );
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
    m_writerParentViewItem->setIcon( 0, QIcon::fromTheme( "media-optical-recordable" ) );
    // spacer item
    QTreeWidgetItem* spacer = new QTreeWidgetItem( m_viewDevices );
    spacer->setFlags( Qt::NoItemFlags );
    m_readerParentViewItem = new QTreeWidgetItem( m_viewDevices, QStringList() << i18n("Read-only Drives") );
    m_readerParentViewItem->setIcon( 0, QIcon::fromTheme( "media-optical" ) );
    // -----------------------------------------

    QFont fBold( m_viewDevices->font() );
    fBold.setBold(true);
    QFont fItalic( m_viewDevices->font() );
    fItalic.setItalic(true);

    foreach( Device::Device* dev, m_deviceManager->allDevices() ) {
        // create the root device item
        QTreeWidgetItem* devRoot = new QTreeWidgetItem( (dev->burner() ? m_writerParentViewItem : m_readerParentViewItem),
                                                        QStringList() << ( dev->vendor() + ' ' + dev->description() ) );
        devRoot->setFont( 0, fBold );

        // create the read-only info items
        QTreeWidgetItem* systemDeviceItem = new QTreeWidgetItem( devRoot, QStringList() << i18n("System device name:") );
        systemDeviceItem->setText( 1, dev->blockDeviceName() );
        systemDeviceItem->setForeground( 0, disabledTextColor );
        systemDeviceItem->setTextAlignment( 0, Qt::AlignRight );

        QTreeWidgetItem* vendorItem = new QTreeWidgetItem( devRoot, systemDeviceItem );
        vendorItem->setText( 0, i18n("Vendor:") );
        vendorItem->setText( 1, dev->vendor() );
        vendorItem->setForeground( 0, disabledTextColor );
        vendorItem->setTextAlignment( 0, Qt::AlignRight );

        QTreeWidgetItem* modelItem = new QTreeWidgetItem( devRoot, vendorItem );
        modelItem->setText( 0, i18n("Description:") );
        modelItem->setText( 1, dev->description() );
        modelItem->setForeground( 0, disabledTextColor );
        modelItem->setTextAlignment( 0, Qt::AlignRight );

        QTreeWidgetItem* versionItem = new QTreeWidgetItem( devRoot, modelItem );
        versionItem->setText( 0, i18n("Firmware:") );
        versionItem->setText( 1, dev->version() );
        versionItem->setForeground( 0, disabledTextColor );
        versionItem->setTextAlignment( 0, Qt::AlignRight );


        // drive type
        // --------------------------------
        QTreeWidgetItem* typeItem = new QTreeWidgetItem( devRoot, versionItem );
        typeItem->setText( 0, i18n("Write Capabilities:") );
        typeItem->setText( 1, K3b::Device::mediaTypeString( dev->writeCapabilities(), true ) );
        typeItem->setToolTip( 1, typeItem->text(1) );
        typeItem->setForeground( 0, disabledTextColor );
        typeItem->setTextAlignment( 0, Qt::AlignRight );

        typeItem = new QTreeWidgetItem( devRoot, typeItem );
        typeItem->setText( 0, i18n("Read Capabilities:") );
        typeItem->setText( 1, K3b::Device::mediaTypeString( dev->readCapabilities(), true ) );
        typeItem->setToolTip( 1, typeItem->text(1) );
        typeItem->setForeground( 0, disabledTextColor );
        typeItem->setTextAlignment( 0, Qt::AlignRight );
        // --------------------------------


        // now add the reader (both interfaces) items
        if( dev->bufferSize() > 0 ) {
            typeItem = new QTreeWidgetItem( devRoot, typeItem );
            typeItem->setText( 0, i18n("Buffer Size:") );
            typeItem->setText( 1, KIO::convertSizeFromKiB(dev->bufferSize()) );
            typeItem->setForeground( 0, disabledTextColor );
            typeItem->setTextAlignment( 0, Qt::AlignRight );
        }


        // now add the writer specific items
        if( dev->burner() ) {
            typeItem = new QTreeWidgetItem( devRoot, typeItem );
            typeItem->setText( 0, i18n("Supports Burnfree:") );
            typeItem->setText( 1, dev->burnfree() ? i18n("yes") : i18n("no") );
            typeItem->setForeground( 0, disabledTextColor );
            typeItem->setTextAlignment( 0, Qt::AlignRight );

            // and at last the write modes
            typeItem = new QTreeWidgetItem( devRoot, typeItem );
            typeItem->setText( 0, i18n("Write modes:") );
            typeItem->setText( 1, K3b::Device::writingModeString(dev->writingModes()) );
            typeItem->setToolTip( 1, typeItem->text(1) );
            typeItem->setForeground( 0, disabledTextColor );
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

    // Show warning message if the current user does not belong to
    // the group device belongs to
    QList<Device::Device*> devices = m_deviceManager->allDevices();
    if( !devices.empty() ) {
        QFileInfo fileInfo( devices.front()->blockDeviceName() );
        m_deviceGroup = fileInfo.group();

        if( m_deviceGroup != "root" ) {
            QVector<gid_t> gids(::getgroups(0, 0));
            ::getgroups(gids.size(), gids.data());

            QSet<QString> groupNames;
            Q_FOREACH( gid_t gid, gids ) {
                if( ::group* g = ::getgrgid( gid ) ) {
                    groupNames.insert( QString::fromLocal8Bit( g->gr_name ) );
                }
            }

            if (!groupNames.contains(m_deviceGroup)) {
		QString messageText = i18n("In order to give K3b full access to the writer device the current user needs be added to a group <em>%1</em>.", m_deviceGroup);
                m_messageWidget->setMessageType(KMessageWidget::Warning);
                m_messageWidget->setText(messageText);
                m_messageWidget->addAction(m_addToGroupAction);
                m_addToGroupAction->setText(i18n("Add"));
                m_messageWidget->animatedShow();
            }
        }
    }
}

void K3b::DeviceWidget::addUserToGroup()
{
    QVariantMap args;
    args["groupName"] = m_deviceGroup;
    args["userName"] = QString::fromLocal8Bit(getpwuid(getuid())->pw_name);

    KAuth::Action action("org.kde.k3b.addtogroup");
    action.setHelperId("org.kde.k3b");
    action.setParentWidget(this);
    action.setArguments(args);

    KAuth::ExecuteJob* job = action.execute();
    connect( job, &KAuth::ExecuteJob::result, [this, job]()
    {
        if( job->error() == KJob::NoError ) {
            m_messageWidget->removeAction(m_addToGroupAction);
            m_messageWidget->setMessageType(KMessageWidget::Information);
            m_messageWidget->setText(i18n("Please relogin to apply the changes."));
        } else {
            m_messageWidget->setMessageType(KMessageWidget::Error);
            m_messageWidget->setText(i18n("Unable to execute the action: %1 (Error code: %2)", job->errorString(), job->error()));
            m_addToGroupAction->setText(i18n("Retry"));
        }
    } );
    job->start();
}
