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

#include <QApplication>
#include <QCursor>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayout>
#include <QMap>
#include <QPixmap>
#include <QPushButton>
#include <QStringList>
#include <QTabWidget>
#include <QToolTip>
#include <QTreeView>

#include <KDebug>
#include <KDialog>
#include <KEditListBox>
#include <KGlobalSettings>
#include <KLocale>





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
    mainGrid->setMargin( 0 );

    m_mainTabWidget = new QTabWidget( this );
    m_rescanButton = new QPushButton( i18n("&Search"), this );
    mainGrid->addWidget( m_mainTabWidget, 0, 0, 1, 2 );
    mainGrid->addWidget( m_rescanButton, 1, 1 );
    mainGrid->setColumnStretch( 0, 1 );
    mainGrid->setRowStretch( 0, 1 );


    // setup program tab
    // ------------------------------------------------------------
    QWidget* programTab = new QWidget( m_mainTabWidget );
    QGridLayout* programTabLayout = new QGridLayout( programTab );
    m_programView = new QTreeView( programTab );
    m_defaultButton = new QPushButton( i18n("Set Default"), programTab );
    m_defaultButton->setToolTip( i18n("Change the versions K3b should use.") );
    m_defaultButton->setWhatsThis( i18n("<p>If K3b finds more than one installed version of a program "
                                        "it will choose one as the <em>default</em>, which will be used "
                                        "to do the work. If you want to change the default, select the "
                                        "desired version and press this button.") );
    QLabel* defaultLabel = new QLabel( i18n("Use the 'Default' button to change the versions K3b should use."),
                                       programTab );
    defaultLabel->setWordWrap( true );
    programTabLayout->addWidget( m_programView, 1, 0, 1, 2 );
    programTabLayout->addWidget( m_defaultButton, 0, 1 );
    programTabLayout->addWidget( defaultLabel, 0, 0 );
    programTabLayout->setColumnStretch( 0, 1 );
    programTabLayout->setRowStretch( 1, 1 );

    m_programView->setModel( m_programModel );
    m_programView->setAllColumnsShowFocus( true );
    m_programView->setRootIsDecorated( false );
    m_programView->setItemsExpandable( false );
    m_programView->header()->setResizeMode( ExternalBinModel::PathColumn, QHeaderView::ResizeToContents );
    m_programView->header()->setResizeMode( ExternalBinModel::VersionColumn, QHeaderView::ResizeToContents );
    
    m_mainTabWidget->addTab( programTab, i18n("Programs") );


    // setup parameters tab
    // ------------------------------------------------------------
    QWidget* parametersTab = new QWidget( m_mainTabWidget );
    QGridLayout* parametersTabLayout = new QGridLayout( parametersTab );
    m_parameterView = new QTreeView( parametersTab );
    QLabel* parametersLabel = new QLabel( i18n("User parameters have to be separated by space."), parametersTab );
    parametersLabel->setWordWrap( true );
    parametersTabLayout->addWidget( m_parameterView, 1, 0 );
    parametersTabLayout->addWidget( parametersLabel, 0, 0 );
    parametersTabLayout->setRowStretch( 1, 1 );

    m_parameterView->setModel( m_parameterModel );
    m_parameterView->setAllColumnsShowFocus( true );
    m_parameterView->setRootIsDecorated( false );
    m_parameterView->setEditTriggers( QAbstractItemView::AllEditTriggers );
    m_parameterView->header()->setResizeMode( ExternalBinParamsModel::ProgramColumn, QHeaderView::ResizeToContents );

    m_mainTabWidget->addTab( parametersTab, i18n("User Parameters") );


    // setup search path tab
    // ------------------------------------------------------------
    QWidget* searchPathTab = new QWidget( m_mainTabWidget );
    m_searchPathBox = new KEditListBox( i18n("Search Path"), searchPathTab );
    m_searchPathBox->setCheckAtEntering( true );
    QLabel* hintLabel = new QLabel( i18n("<qt><b>Hint:</b> to force K3b to use another than the "
                                         "default name for the executable specify it in the search path.</qt>"),
                                    searchPathTab );
    hintLabel->setWordWrap( true );
    QGridLayout* searchPathTabLayout = new QGridLayout( searchPathTab );
    searchPathTabLayout->addWidget( m_searchPathBox, 0, 0 );
    searchPathTabLayout->addWidget( hintLabel, 1, 0 );
    searchPathTabLayout->setRowStretch( 0, 1 );

    m_mainTabWidget->addTab( searchPathTab, i18n("Search Path") );

    connect( m_rescanButton, SIGNAL(clicked()), this, SLOT(rescan()) );
    connect( m_defaultButton, SIGNAL(clicked()), this, SLOT(slotSetDefaultButtonClicked()) );
    connect( m_programView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotProgramSelectionChanged(QModelIndex,QModelIndex)) );

    slotProgramSelectionChanged( QModelIndex(), QModelIndex() );
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


void K3b::ExternalBinWidget::slotSetDefaultButtonClicked()
{
    // check if we are on a binItem
    QModelIndex index = m_programView->currentIndex();
    m_programModel->setDefault( index );
}


void K3b::ExternalBinWidget::slotProgramSelectionChanged( const QModelIndex& current, const QModelIndex& /*previous*/ )
{
    if( current.isValid() && m_programModel->binForIndex( current ) ) {
        if( m_programModel->isDefault( current ) )
            m_defaultButton->setEnabled(false);
        else
            m_defaultButton->setEnabled(true);
    }
    else
        m_defaultButton->setEnabled(false);
}

#include "k3bexternalbinwidget.moc"
