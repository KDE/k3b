/*
    SPDX-FileCopyrightText: 2005-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bprojectplugindialog.h"

#include "k3bprojectplugin.h"

#include <QWidget>


K3b::ProjectPluginDialog::ProjectPluginDialog( K3b::ProjectPlugin* plugin, K3b::Doc* doc, QWidget* parent )
    : K3b::InteractionDialog( parent,
                            QString(),
                            QString(),
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            plugin->metaObject()->className() )
{
    m_pluginGui = plugin->createGUI( doc, this );
    Q_ASSERT( m_pluginGui );
    Q_ASSERT( m_pluginGui->qWidget() );
    setMainWidget( m_pluginGui->qWidget() );
    setTitle( m_pluginGui->title(), m_pluginGui->subTitle() );
}


K3b::ProjectPluginDialog::~ProjectPluginDialog()
{
}


void K3b::ProjectPluginDialog::slotStartClicked()
{
    m_pluginGui->activate();
}


void K3b::ProjectPluginDialog::saveSettings( KConfigGroup config )
{
    m_pluginGui->saveSettings( config );
}


void K3b::ProjectPluginDialog::loadSettings( const KConfigGroup& config )
{
    m_pluginGui->readSettings( config );
}

#include "moc_k3bprojectplugindialog.cpp"
