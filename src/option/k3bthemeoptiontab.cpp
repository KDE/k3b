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


#include "k3bthemeoptiontab.h"

#include "k3bthememanager.h"

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <klistview.h>
#include <kio/global.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <kstandarddirs.h>
#include <ktar.h>
#include <kurlrequesterdlg.h>

#include <qlabel.h>
#include <qfile.h>
#include <qfileinfo.h>


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
  connect( m_buttonInstallTheme, SIGNAL(clicked()),
	   this, SLOT(slotInstallTheme()) );
  connect( m_buttonRemoveTheme, SIGNAL(clicked()),
	   this, SLOT(slotRemoveTheme()) );
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

    QFileInfo fi( item->theme->path() );
    m_buttonRemoveTheme->setEnabled( fi.isWritable() ); 
  }
}


void K3bThemeOptionTab::slotInstallTheme()
{
  KURL themeURL = KURLRequesterDlg::getURL( QString::null, this,
					    i18n("Drag or Type Theme URL") );

  if( themeURL.url().isEmpty() )
    return;

  QString themeTmpFile;
  // themeTmpFile contains the name of the downloaded file

  if( !KIO::NetAccess::download( themeURL, themeTmpFile, this ) ) {
    QString sorryText;
    if (themeURL.isLocalFile())
       sorryText = i18n("Unable to find the icon theme archive %1!");
    else
       sorryText = i18n("Unable to download the icon theme archive!\n"
                        "Please check that address %1 is correct.");
    KMessageBox::sorry( this, sorryText.arg(themeURL.prettyURL()) );
    return;
  }

  // check if the archive contains a dir with a k3b.theme file
  KTar archive( themeTmpFile );
  archive.open(IO_ReadOnly);
  const KArchiveDirectory* themeDir = archive.directory();
  QStringList entries = themeDir->entries();
  bool validThemeArchive = false;
  if( entries.count() > 0 ) {
    if( themeDir->entry(entries.first())->isDirectory() ) {
      const KArchiveDirectory* subDir = dynamic_cast<const KArchiveDirectory*>( themeDir->entry(entries.first()) );
      if( subDir && subDir->entry( "k3b.theme" ) )
	validThemeArchive = true;
    }
  }

  if( !validThemeArchive ) {
    KMessageBox::error( this, i18n("The file is not a valid K3b theme archive!") );
  }
  else {
    // install the theme
    archive.directory()->copyTo( locateLocal( "data", "k3b/pics/" ) );
  }

  archive.close();
  KIO::NetAccess::removeTempFile(themeTmpFile);

  readSettings();
}


void K3bThemeOptionTab::slotRemoveTheme()
{
  ThemeViewItem* item = (ThemeViewItem*)m_viewTheme->selectedItem();
  if( item ) {
    QString question=i18n("<qt>Are you sure you want to remove the "
			  "<strong>%1</strong> icon theme?<br>"
			  "<br>"
			  "This will delete the files installed by this theme.</qt>").
      arg(item->text(0));

    if( KMessageBox::questionYesNo( this, question, i18n("Confirmation")) != KMessageBox::Yes )
      return;

    K3bTheme* theme = item->theme;
    delete item;
    QString path = theme->path();

    // delete k3b.theme file to avoid it to get loaded
    QFile::remove( path + "/k3b.theme" );
    
    // reread the themes (this will also set the default theme in case we delete the 
    // selected one)
    readSettings();

    // delete the theme data itself
    KIO::del( path, false, false );
  }
}

#include "k3bthemeoptiontab.moc"
