/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bpluginoptiontab.h"


#include <k3bpluginmanager.h>
#include <k3bpluginfactory.h>
#include <k3bpluginconfigwidget.h>

#include <k3bcore.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <klistview.h>
#include <kdialogbase.h>
#include <kmessagebox.h>
#include <kconfig.h>

#include <qstringlist.h>
#include <qpushbutton.h>


class K3bPluginOptionTab::PluginViewItem : public KListViewItem
{
public:
  PluginViewItem( K3bPluginFactory* factory, KListViewItem* parent )
    : KListViewItem( parent ),
      pluginFactory(factory) {
    setText( 0, factory->name() );
    if( !factory->author().isEmpty() ) {
      if( factory->email().isEmpty() )
	setText( 1, factory->author() );
      else
	setText( 1, factory->author() + " <" + factory->email() + ">" );
    }
    setText( 2, factory->version() );
    setText( 3, factory->comment() );
  }

  K3bPluginFactory* pluginFactory;
};



K3bPluginOptionTab::K3bPluginOptionTab( QWidget* parent, const char* name )
  : base_K3bPluginOptionTab( parent, name )
{
  connect( m_buttonConfigure, SIGNAL(clicked()), this, SLOT(slotConfigureButtonClicked()) );
  connect( m_viewPlugins, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()) );
}


K3bPluginOptionTab::~K3bPluginOptionTab()
{
}


void K3bPluginOptionTab::readSettings()
{
  m_viewPlugins->clear();
  QStringList groups = k3bpluginmanager->groups();
  for( QStringList::const_iterator it = groups.begin();
       it != groups.end(); ++it ) {
    const QString& group = *it;

    KListViewItem* groupViewItem = new KListViewItem( m_viewPlugins, 
						      m_viewPlugins->lastChild(),
						      group );
    QPtrList<K3bPluginFactory> fl = k3bpluginmanager->factories( group );
    for( QPtrListIterator<K3bPluginFactory> fit( fl ); fit.current(); ++fit )
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
  if( PluginViewItem* pi = dynamic_cast<PluginViewItem*>( item ) ) {
    KDialogBase dlg( this, 
		     "pluginConfigDlg", 
		     true,
		     i18n("Configure plugin %1").arg( pi->pluginFactory->name() ) );
    
    K3bPluginConfigWidget* configWidget = pi->pluginFactory->createConfigWidget( &dlg );
    if( configWidget ) {
      dlg.setMainWidget( configWidget );
      connect( &dlg, SIGNAL(applyClicked()), configWidget, SLOT(saveConfig()) );
      connect( &dlg, SIGNAL(okClicked()), configWidget, SLOT(saveConfig()) );
      configWidget->loadConfig();
      dlg.exec();
      delete configWidget;
    }
    else {
      KMessageBox::sorry( this, i18n("No settings available for plugin %1.").arg( pi->pluginFactory->name() ) );
    }
  }
}


void K3bPluginOptionTab::slotSelectionChanged()
{
  m_buttonConfigure->setEnabled( dynamic_cast<PluginViewItem*>( m_viewPlugins->selectedItem() ) != 0 );
}

#include "k3bpluginoptiontab.moc"
