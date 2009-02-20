/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bprojectplugindialog.h"

#include <k3bprojectplugin.h>

#include <qwidget.h>


K3bProjectPluginDialog::K3bProjectPluginDialog( K3bProjectPlugin* plugin, K3bDoc* doc, QWidget* parent )
    : K3bInteractionDialog( parent,
                            QString(),
                            QString(),
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            plugin->metaObject()->className() ),
      m_plugin(plugin)
{
    m_pluginGui = plugin->createGUI( doc, this );
    Q_ASSERT( m_pluginGui );
    Q_ASSERT( m_pluginGui->qWidget() );
    setMainWidget( m_pluginGui->qWidget() );
    setTitle( m_pluginGui->title(), m_pluginGui->subTitle() );
}


K3bProjectPluginDialog::~K3bProjectPluginDialog()
{
}


void K3bProjectPluginDialog::slotStartClicked()
{
    m_pluginGui->activate();
}


void K3bProjectPluginDialog::saveUserDefaults( KConfigGroup config )
{
    m_pluginGui->saveSettings( config );
}


void K3bProjectPluginDialog::loadUserDefaults( const KConfigGroup& config )
{
    m_pluginGui->readSettings( config );
}


void K3bProjectPluginDialog::loadK3bDefaults()
{
    m_pluginGui->loadDefaults();
}

#include "k3bprojectplugindialog.moc"
