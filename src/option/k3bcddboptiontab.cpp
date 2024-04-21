/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bcddboptiontab.h"

#include <KCModule>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QLabel>
#include <QHBoxLayout>


K3b::CddbOptionTab::CddbOptionTab( QWidget* parent )
    : QWidget( parent )
{
    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    m_cddbKcm = 0;

    const auto result = KPluginFactory::instantiatePlugin<KCModule>(KPluginMetaData(QStringLiteral("plasma/kcms/systemsettings_qwidgets/kcm_cddb")), this);

    if (result) {
        m_cddbKcm = result.plugin;
        m_cddbKcm->widget()->layout()->setContentsMargins( 0, 0, 0, 0 );
        layout->addWidget( m_cddbKcm->widget() );
    } else {
        QLabel* label = new QLabel( i18n( "Unable to load KCDDB configuration module." ), this );
        label->setAlignment( Qt::AlignCenter );
        layout->addWidget( label );
    }
}


K3b::CddbOptionTab::~CddbOptionTab()
{
}


void K3b::CddbOptionTab::readSettings()
{
    if ( m_cddbKcm ) {
        m_cddbKcm->load();
    }
}


void K3b::CddbOptionTab::apply()
{
    if ( m_cddbKcm ) {
        m_cddbKcm->save();
    }
}

#include "moc_k3bcddboptiontab.cpp"
