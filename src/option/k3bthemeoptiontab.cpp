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


#include "k3bthemeoptiontab.h"

#include "k3bthememanager.h"

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <klistview.h>

#include <qlabel.h>


class K3bThemeOptionTab::Private
{
public:
};


class ThemeViewItem : public KListViewItem 
{
public:
  ThemeViewItem( K3bTheme* theme_, QListView* parent, QListViewItem* after )
    : KListViewItem( parent, after ),
      theme(theme_) {
    setText( 0, theme->name() );
    setText( 1, theme->author() );
    setText( 2, theme->version() );
    setText( 3, theme->comment() );
  }

  K3bTheme* theme;
};

K3bThemeOptionTab::K3bThemeOptionTab(QWidget *parent, const char *name )
  : base_K3bThemeOptionTab(parent,name)
{
  d = new Private();

  connect( m_viewTheme, SIGNAL(selectionChanged()),
	   this, SLOT(selectionChanged()) );
}


K3bThemeOptionTab::~K3bThemeOptionTab()
{
  delete d;
}


void K3bThemeOptionTab::readSettings()
{
  m_viewTheme->clear();

  k3bthememanager->loadThemes();

  const QPtrList<K3bTheme>& themes = k3bthememanager->themes();
  for( QPtrListIterator<K3bTheme> it( themes ); it.current(); ++it ) {
    ThemeViewItem* item = new ThemeViewItem( it.current(), m_viewTheme, m_viewTheme->lastItem() );
    if( it.current() == k3bthememanager->currentTheme() )
      m_viewTheme->setSelected( item, true );
  }
}


bool K3bThemeOptionTab::saveSettings()
{
  ThemeViewItem* item = (ThemeViewItem*)m_viewTheme->selectedItem();
  if( item )
    k3bthememanager->setCurrentTheme( item->theme );

  return true;
}


void K3bThemeOptionTab::selectionChanged()
{
  ThemeViewItem* item = (ThemeViewItem*)m_viewTheme->selectedItem();
  if( item ) {
    m_centerPreviewLabel->setText( i18n("K3b - The CD/DVD Kreator") );
    m_centerPreviewLabel->setPaletteBackgroundColor( item->theme->backgroundColor() );
    m_centerPreviewLabel->setPaletteForegroundColor( item->theme->foregroundColor() );
    m_leftPreviewLabel->setPixmap( item->theme->pixmap( "k3bprojectview_left_short" ) );
    m_rightPreviewLabel->setPixmap( item->theme->pixmap( "k3bprojectview_right" ) );
  }
}

#include "k3bthemeoptiontab.moc"
