/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bpluginoptiontab.h"


#include <k3bpluginmanager.h>
#include <k3bplugin.h>
#include <k3bpluginconfigwidget.h>

#include <k3bcore.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <klistview.h>
#include <kdialogbase.h>
#include <kconfig.h>

#include <qstringlist.h>
#include <qpushbutton.h>


class K3bPluginOptionTab::PluginViewItem : public KListViewItem
{
public:
  PluginViewItem( K3bPlugin* p, KListViewItem* parent )
    : KListViewItem( parent ),
      plugin(p) {
    const K3bPluginInfo& info = p->pluginInfo();
    setText( 0, info.name() );
    if( !info.author().isEmpty() ) {
      if( info.email().isEmpty() )
	setText( 1, info.author() );
      else
	setText( 1, info.author() + " <" + info.email() + ">" );
    }
    setText( 2, info.version() );
    setText( 3, info.comment() );
    setText( 4, info.licence() );
  }

  K3bPlugin* plugin;
};



K3bPluginOptionTab::K3bPluginOptionTab( QWidget* parent, const char* name )
  : base_K3bPluginOptionTab( parent, name )
{
  m_viewPlugins->setShadeSortColumn( false );

  connect( m_viewPlugins, SIGNAL(doubleClicked(QListViewItem*, const QPoint&, int)), this, SLOT(slotConfigureButtonClicked()) );
  connect( m_buttonConfigure, SIGNAL(clicked()), this, SLOT(slotConfigureButtonClicked()) );
  connect( m_viewPlugins, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()) );
}


K3bPluginOptionTab::~K3bPluginOptionTab()
{
}


void K3bPluginOptionTab::readSettings()
{
  m_viewPlugins->clear();
  QStringList groups = k3bcore->pluginManager()->groups();
  for( QStringList::const_iterator it = groups.begin();
       it != groups.end(); ++it ) {
    const QString& group = *it;

    KListViewItem* groupViewItem = new KListViewItem( m_viewPlugins,
						      m_viewPlugins->lastChild(),
						      group );
    QPtrList<K3bPlugin> fl = k3bcore->pluginManager()->plugins( group );
    for( QPtrListIterator<K3bPlugin> fit( fl ); fit.current(); ++fit )
      (void)new PluginViewItem( fit.current(), groupViewItem );

    groupViewItem->setOpen(true);
  }

  slotSelectionChanged();
}


bool K3bPluginOptionTab::saveSettings()
{
  return true;
}


void K3bPluginOptionTab::slotConfigureButtonClicked()
{
  QListViewItem* item = m_viewPlugins->selectedItem();
  if( PluginViewItem* pi = dynamic_cast<PluginViewItem*>( item ) )
    k3bcore->pluginManager()->execPluginDialog( pi->plugin, this );
}


void K3bPluginOptionTab::slotSelectionChanged()
{
  m_buttonConfigure->setEnabled( dynamic_cast<PluginViewItem*>( m_viewPlugins->selectedItem() ) != 0 );
}

#include "k3bpluginoptiontab.moc"
