/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "k3bexternalbinoptiontab.h"
#include "k3bexternalbinmanager.h"
#include "k3bexternalbinwidget.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QLabel>
#include <QVBoxLayout>


K3b::ExternalBinOptionTab::ExternalBinOptionTab( K3b::ExternalBinManager* manager, QWidget* parent )
    : QWidget( parent )
{
    m_manager = manager;

    m_externalBinWidget = new K3b::ExternalBinWidget( manager, this );

    QLabel* m_labelInfo = new QLabel( this );
    m_labelInfo->setWordWrap( true );
    m_labelInfo->setText( "<p>" + i18n( "Specify the paths to the external programs that K3b needs to work properly, "
                                        "or press \"Refresh\" to let K3b search for the programs." ) + "</p>" );

    QVBoxLayout* frameLayout = new QVBoxLayout( this );
    frameLayout->setContentsMargins( 0, 0, 0, 0 );
    frameLayout->addWidget( m_labelInfo );
    frameLayout->addWidget( m_externalBinWidget );
}


K3b::ExternalBinOptionTab::~ExternalBinOptionTab()
{
}


void K3b::ExternalBinOptionTab::readSettings()
{
    m_externalBinWidget->load();
}


void K3b::ExternalBinOptionTab::saveSettings()
{
    m_externalBinWidget->save();
}

#include "moc_k3bexternalbinoptiontab.cpp"
