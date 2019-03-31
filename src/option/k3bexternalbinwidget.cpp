/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bexternalbinwidget.h"
#include "k3bexternalbinmanager.h"
#include "k3bexternalbinmodel.h"
#include "k3bexternalbinparamsmodel.h"
#include "k3bexternalbinpermissionmodel.h"
#include "config-k3b.h"

#include <KAuth>
#include <KLocalizedString>
#include <KMessageBox>
#include <KEditListWidget>

#include <QDebug>
#include <QItemSelectionModel>
#include <QMap>
#include <QStringList>
#include <QCursor>
#include <QPixmap>
#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QToolTip>
#include <QTreeView>
#include <QVBoxLayout>

#include <grp.h>



// ///////////////////////////////////////////////////////////
//
// K3BEXTERNALBINWIDGET
//
// //////////////////////////////////////////////////////////


K3b::ExternalBinWidget::ExternalBinWidget( K3b::ExternalBinManager* manager, QWidget* parent )
    : QWidget( parent ),
      m_manager( manager ),
      m_programModel( new ExternalBinModel( manager, this ) ),
      m_parameterModel( new ExternalBinParamsModel( manager, this ) )
{
    QGridLayout* mainGrid = new QGridLayout( this );
    mainGrid->setContentsMargins( 0, 0, 0, 0 );

    m_mainTabWidget = new QTabWidget( this );
    m_rescanButton = new QPushButton( QIcon::fromTheme( "view-refresh" ), i18n( "Refresh" ), this );
    mainGrid->addWidget( m_mainTabWidget, 0, 0, 1, 3 );
    mainGrid->addWidget( m_rescanButton, 1, 2 );
    mainGrid->setColumnStretch( 0, 1 );
    mainGrid->setRowStretch( 0, 1 );

    // setup program tab
    // ------------------------------------------------------------
    QWidget* programTab = new QWidget( m_mainTabWidget );
    m_programView = new QTreeView( programTab );
    m_programView->setModel( m_programModel );
    m_programView->setAllColumnsShowFocus( true );
    m_programView->setRootIsDecorated( false );
    m_programView->setItemsExpandable( false );
    m_programView->header()->setSectionResizeMode( ExternalBinModel::PathColumn, QHeaderView::ResizeToContents );
    m_programView->header()->setSectionResizeMode( ExternalBinModel::VersionColumn, QHeaderView::ResizeToContents );
    m_programView->setWhatsThis( i18n("<p>If K3b finds more than one installed version of a program "
                                      "it will choose one as the <em>default</em>, which will be used "
                                      "to do the work. If you want to change the default, check "
                                      "desired version on the list.") );

    QVBoxLayout* programTabLayout = new QVBoxLayout( programTab );
    programTabLayout->addWidget( m_programView );

    m_mainTabWidget->addTab( programTab, i18n("Programs") );


    // setup parameters tab
    // ------------------------------------------------------------
    QWidget* parametersTab = new QWidget( m_mainTabWidget );
    QLabel* parametersLabel = new QLabel( i18n("User parameters have to be separated by space."), parametersTab );
    parametersLabel->setWordWrap( true );
    m_parameterView = new QTreeView( parametersTab );
    m_parameterView->setModel( m_parameterModel );
    m_parameterView->setAllColumnsShowFocus( true );
    m_parameterView->setRootIsDecorated( false );
    m_parameterView->setEditTriggers( QAbstractItemView::AllEditTriggers );
    m_parameterView->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

    QVBoxLayout* parametersTabLayout = new QVBoxLayout( parametersTab );
    parametersTabLayout->addWidget( parametersLabel );
    parametersTabLayout->addWidget( m_parameterView, 1 );

    m_mainTabWidget->addTab( parametersTab, i18n("User Parameters") );


    // setup permissions tab
    // ------------------------------------------------------------
    QWidget* permissionsTab = new QWidget( m_mainTabWidget );
    QLabel* permissionsLabel = new QLabel( i18n("Check the programs whose permissions you want to be changed:"), permissionsTab );
    permissionsLabel->setWordWrap( true );
    m_permissionModel = new ExternalBinPermissionModel( *manager, permissionsTab );
    m_permissionView = new QTreeView( permissionsTab );
    m_permissionView->setModel( m_permissionModel );
    m_permissionView->setAllColumnsShowFocus( true );
    m_permissionView->setRootIsDecorated( false );
    m_permissionView->header()->setSectionResizeMode( ExternalBinPermissionModel::ProgramColumn, QHeaderView::ResizeToContents );
    m_changePermissionsButton = new QPushButton( QIcon::fromTheme("dialog-password"), i18n( "Change Permissions..." ), this );
    QVBoxLayout* permissionsTabLayout = new QVBoxLayout( permissionsTab );
    permissionsTabLayout->addWidget( permissionsLabel );
    permissionsTabLayout->addWidget( m_permissionView );
    permissionsTabLayout->addWidget( m_changePermissionsButton );
    m_mainTabWidget->addTab( permissionsTab, i18n("Permissions") );


    // setup search path tab
    // ------------------------------------------------------------
    QWidget* searchPathTab = new QWidget( m_mainTabWidget );
    m_searchPathBox = new KEditListWidget( searchPathTab );
    m_searchPathBox->setCheckAtEntering( true );
    QLabel* hintLabel = new QLabel( i18n("<qt><b>Hint:</b> to force K3b to use another than the "
                                         "default name for the executable specify it in the search path.</qt>"),
                                    searchPathTab );
    hintLabel->setWordWrap( true );
    QVBoxLayout* searchPathTabLayout = new QVBoxLayout( searchPathTab );
    searchPathTabLayout->addWidget( m_searchPathBox, 1 );
    searchPathTabLayout->addWidget( hintLabel );

    m_mainTabWidget->addTab( searchPathTab, i18n("Search Path") );

    connect( m_changePermissionsButton, SIGNAL(clicked()), SLOT(slotChangePermissions()) );
    connect( m_permissionModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(slotPermissionModelChanged()) );
    connect( m_permissionModel, SIGNAL(modelReset()), SLOT(slotPermissionModelChanged()) );
    connect( m_rescanButton, SIGNAL(clicked(bool)), this, SLOT(rescan()) );

    qRegisterMetaType<HelperProgramItem>();
    qRegisterMetaTypeStreamOperators<HelperProgramItem>("K3b::HelperProgramItem");

    while (::group *g = ::getgrent()) {
        const QString groupName = QString::fromLocal8Bit(g->gr_name);
        if (groupName == "cdrom" ||
            groupName == "optical" ||
            groupName == "operator" ) {
            m_permissionModel->setBurningGroup(groupName);
        }
    }
    ::endgrent();
}


K3b::ExternalBinWidget::~ExternalBinWidget()
{
}

void K3b::ExternalBinWidget::rescan()
{
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    saveSearchPath();
    m_manager->search();
    load();
    QApplication::restoreOverrideCursor();
}


void K3b::ExternalBinWidget::load()
{
    m_programModel->reload();
    m_programView->expandAll();
    m_parameterModel->reload();

    // load search path
    m_searchPathBox->clear();
    m_searchPathBox->insertStringList( m_manager->searchPath() );
}


void K3b::ExternalBinWidget::save()
{
    saveSearchPath();

    m_programModel->save();
    m_parameterModel->save();
}


void K3b::ExternalBinWidget::saveSearchPath()
{
    m_manager->setSearchPath( m_searchPathBox->items() );
}


void K3b::ExternalBinWidget::slotPermissionModelChanged()
{
    m_changePermissionsButton->setEnabled(m_permissionModel->changesNeeded());
}


void K3b::ExternalBinWidget::slotChangePermissions()
{
    KAuth::Action action("org.kde.k3b.updatepermissions");
    action.setHelperId("org.kde.k3b");
    action.setParentWidget(this);

    QVariantMap args;

    // Set burning group name as first argument
    args["burningGroup"] = m_permissionModel->burningGroup();

    // Set devices list as second argument
    args["devices"] = QStringList();

    // Set programs list as third argument
    QVariantList programs;
    Q_FOREACH(const HelperProgramItem& item, m_permissionModel->selectedPrograms()) {
        programs << QVariant::fromValue(item);
    }
    args["programs"] = programs;

    action.setArguments(args);

    KAuth::ExecuteJob* job = action.execute();
    connect( job, &KAuth::ExecuteJob::result, [this, job]()
    {
        if( job->error() == KJob::NoError ) {
            // Success!!
            QStringList updated = job->data()["updated"].toStringList();
            QStringList failedToUpdate = job->data()["failedToUpdate"].toStringList();
            qDebug() << "Objects updated: " << updated;
            qDebug() << "Objects failed to update: " << failedToUpdate;

            if (!failedToUpdate.isEmpty()) {
                KMessageBox::errorList(this, i18n("Following programs could not be updated:"), failedToUpdate);
            }

            m_permissionModel->update();
        } else {
            KMessageBox::error(this, i18n("Unable to execute the action: %1", job->errorString()));
        }
    } );
    job->start();
}


