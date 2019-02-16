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

#include "k3bcddboptiontab.h"

#include <KCModule>
#include <KService>
#include <KServiceTypeTrader>
#include <KLocalizedString>

#include <QLabel>
#include <QHBoxLayout>


K3b::CddbOptionTab::CddbOptionTab( QWidget* parent )
    : QWidget( parent )
{
    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    m_cddbKcm = 0;
    if ( KService::Ptr service = KService::serviceByStorageId( "libkcddb.desktop" ) )
        m_cddbKcm = service->createInstance<KCModule>( this );
    if (!m_cddbKcm) {
        KService::List services = KServiceTypeTrader::self()->query( "KCModule", "[X-KDE-Library] == 'kcm_cddb'" );
        if ( !services.isEmpty() ) {
            m_cddbKcm = services.first()->createInstance<KCModule>( this );
        }
    }

    if ( m_cddbKcm ) {
        m_cddbKcm->layout()->setContentsMargins( 0, 0, 0, 0 );
        layout->addWidget( m_cddbKcm );
    }
    else {
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


