/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdataurladdingdialog.h"

#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qdir.h>
#include <qfileinfo.h>

#include <k3bbusywidget.h>
#include <k3bdatadoc.h>
#include <k3bdiritem.h>
#include <k3bcore.h>
#include <k3bfileitem.h>
#include <k3bmultichoicedialog.h>
#include <k3bvalidators.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kurl.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kstdguiitem.h>


K3bDataUrlAddingDialog::K3bDataUrlAddingDialog( QWidget* parent, const char* name )
  : KDialogBase( Plain,
		 0,
		 parent,
		 name,
		 true,
		 i18n("Please be patient..."),
		 Cancel,
		 Cancel,
		 true ),
    m_bExistingItemsReplaceAll(false),
    m_bExistingItemsIgnoreAll(false),
    m_iAddHiddenFiles(0),
    m_iAddSystemFiles(0)
{
  QWidget* page = plainPage();
  QGridLayout* grid = new QGridLayout( page );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

//   QLabel* pixLabel = new QLabel( page );
//   pixLabel->setPixmap( KGlobal::iconLoader()->loadIcon( "editcopy", KIcon::NoGroup, 32 ) );
//   pixLabel->setScaledContents( false );

  m_infoLabel = new QLabel( page );
  m_busyWidget = new K3bBusyWidget( page );

  //  grid->addMultiCellWidget( pixLabel, 0, 1, 0, 0 );
  grid->addWidget( m_infoLabel, 0, 0 );
  grid->addWidget( m_busyWidget, 1, 0 );
}


K3bDataUrlAddingDialog::~K3bDataUrlAddingDialog()
{
}


int K3bDataUrlAddingDialog::addUrls( const KURL::List& urls, 
				     K3bDirItem* dir,
				     QWidget* parent )
{
  K3bDataUrlAddingDialog dlg( parent );
  dlg.m_infoLabel->setText( i18n("Adding files to project \"%1\"...").arg(dir->doc()->URL().fileName()) );
  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it )
    dlg.m_urlQueue.append( qMakePair( *it, dir ) );

  dlg.slotAddUrls();
  int ret = QDialog::Accepted;
  if( !dlg.m_urlQueue.isEmpty() ) {
    dlg.m_busyWidget->showBusy(true);
    ret = dlg.exec();
  }

  QString message;
  if( !dlg.m_unreadableFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("Insufficient permissions to read the following files") )
      .arg( dlg.m_unreadableFiles.join( "<br>" ) );
  if( !dlg.m_notFoundFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("Unable to find the following files") )
      .arg( dlg.m_notFoundFiles.join( "<br>" ) );
  if( !dlg.m_nonLocalFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("No non-local files supported") )
      .arg( dlg.m_unreadableFiles.join( "<br>" ) );
  if( !dlg.m_tooBigFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("It is not possible to add files bigger than 4 GB") )
      .arg( dlg.m_tooBigFiles.join( "<br>" ) );

  if( !message.isEmpty() )
    KMessageBox::detailedSorry( parent, i18n("Some files could not be added to the project."), message );

  return ret;
}


void K3bDataUrlAddingDialog::slotAddUrls()
{
  // add next url
  KURL url = m_urlQueue.first().first;
  K3bDirItem* dir = m_urlQueue.first().second;
  m_urlQueue.remove( m_urlQueue.begin() );

  bool valid = true;
  QFileInfo f( url.path() );
  if( !url.isLocalFile() ) {
    valid = false;
    m_nonLocalFiles.append( url.path() );
  }

  // QFileInfo::exists and QFileInfo::isReadable return false for broken symlinks :(
  // and symlinks are always readable and can always be added to a project
  else if( !f.isSymLink() ) {
    if( !f.isReadable() ) {
      valid = false;
      m_unreadableFiles.append( url.path() );
    }
    if( !f.exists() ) {
      valid = false;
      m_notFoundFiles.append( url.path() );
    }
    if( f.isFile() && K3b::filesize( url ) > 4LL*1024LL*1024LL*1024LL ) {
      valid = false;
      m_tooBigFiles.append( url.path() );
    }
  }


  QString newName = url.fileName();
  K3bDirItem* newDirItem = 0;

  //
  // The source is valid. Now check if the project already contains a file with that name
  // and if so handle it properly
  //
  if( valid ) {
    if( K3bDataItem* oldItem = dir->find( newName ) ) {
      //
      // reuse an existing dir
      //
      if( oldItem->isDir() && f.isDir() )
	newDirItem = dynamic_cast<K3bDirItem*>(oldItem);

      //
      // we cannot replace files in the old session with dirs and vice versa (I think)
      // files are handled in K3bFileItem constructor and dirs handled above
      //
      else if( oldItem->isFromOldSession() &&
	       f.isDir() != oldItem->isDir() ) {
	if( !getNewName( newName, dir, newName ) )
	  valid = false;
      }

      else if( m_bExistingItemsIgnoreAll )
	valid = false;

      else if( m_bExistingItemsReplaceAll ) {
	// if we replace an item from an old session the K3bFileItem constructor takes care
	// of replacing the item
	if( !oldItem->isFromOldSession() )
	  delete oldItem;
      }

      //
      // Let the user choose
      //
      else {
	switch( K3bMultiChoiceDialog::choose( i18n("File already exists"),
					      i18n("<p>File <em>%1</em> already exists in "
						   "project folder <em>%2</em>.")
					      .arg(newName)
					      .arg("/" + dir->k3bPath()),
					      this,
					      0,
					      6,
					      KGuiItem( i18n("Replace"), 
							QString::null,
							i18n("Replace the existing file") ),
					      KGuiItem( i18n("Replace All"),
							QString::null,
							i18n("Always replace existing files") ),
					      KGuiItem( i18n("Ignore"),
							QString::null,
							i18n("Keep the existing file") ),
					      KGuiItem( i18n("Ignore All"),
							QString::null,
							i18n("Always keep the existing file") ),
					      KGuiItem( i18n("Rename"),
							QString::null,
							i18n("Rename the new file") ),
					      KStdGuiItem::cancel() ) ) {
	case 2: // replace all
	  m_bExistingItemsReplaceAll = true;
	  // fallthrough
	case 1: // replace
	  // if we replace an item from an old session the K3bFileItem constructor takes care
	  // of replacing the item
	  if( !oldItem->isFromOldSession() )
	    delete oldItem;
	  break;
	case 4: // ignore all
	  m_bExistingItemsIgnoreAll = true;
	  // fallthrough
	case 3: // ignore
	  valid = false;
	  break;
	case 5: // rename
	  if( !getNewName( newName, dir, newName ) )
	    valid = false;
	  break;
	case 6: // cancel
	  slotCancel();
	  return;
	}
      }
    }
  }


  //
  // One more thing to warn the user about: We cannot follow links to folders since that
  // would change the doc. So we simply ask the user what to do with a link to a folder
  //
  if( valid ) {
    if( f.isDir() && f.isSymLink() ) {

      QString resolved = K3b::resolveLink( f.absFilePath() );

      // let's see if this link starts a loop
      // that means if it points to some folder above this one
      // if so we cannot follow it anyway
      if( !f.absFilePath().startsWith( resolved ) &&
	  ( dir->doc()->isoOptions().followSymbolicLinks() ||
	    KMessageBox::warningYesNo( this,
				       i18n("<p>'%1' is a symbolic link to folder '%2'."
					    "<p>If you intend to make K3b follow symbolic links you should consider letting K3b do this now "
					    "since K3b will not be able to do so afterwards because symbolic links to folders inside a "
					    "K3b project cannot be resolved."
					    "<p><b>If you do not intend to enable the option <em>follow symbolic links</em> you may savely "
					    "ignore this warning and choose to add the link to the project.</b>")
				       .arg(f.absFilePath())
				       .arg(resolved ),
				       i18n("Adding link to folder"),
				       i18n("Follow link now"),
				       i18n("Add symbolic link to project"),
				       "ask_to_follow_link_to_folder" ) == KMessageBox::Yes ) ) {
	f.setFile( resolved );
      }
    }
  }


  //
  // Project valid also (we overwrite or renamed)
  // now create the new item
  //
  if( valid ) {
    if( f.isDir() && !f.isSymLink() ) {
      if( !newDirItem ) { // maybe we reuse an already existing dir
	newDirItem = new K3bDirItem( newName , dir->doc(), dir );
	newDirItem->setLocalPath( url.path() ); // HACK: see k3bdiritem.h
      }

      QDir newDir( f.absFilePath() );      

      int dirFilter = QDir::All;
      if( checkForHiddenFiles( newDir ) )
	dirFilter |= QDir::Hidden;
      if( checkForSystemFiles( newDir ) )
	dirFilter |= QDir::System;
	
      QStringList dlist = newDir.entryList( dirFilter );
      dlist.remove(".");
      dlist.remove("..");

      for( QStringList::Iterator it = dlist.begin(); it != dlist.end(); ++it ) {
	m_urlQueue.append( qMakePair( KURL::fromPathOrURL(f.absFilePath() + "/" + *it), newDirItem ) );
      }
    }
    else {
      (void)new K3bFileItem( url.path(), dir->doc(), dir, newName );
    }
  }

  if( m_urlQueue.isEmpty() )
    accept();
  else
    QTimer::singleShot( 0, this, SLOT(slotAddUrls()) );
}


bool K3bDataUrlAddingDialog::getNewName( const QString& oldName, K3bDirItem* dir, QString& newName )
{
  bool ok = true;
  newName = oldName;
  QValidator* validator = K3bValidators::iso9660Validator( false, this );
  do {
    newName = KInputDialog::getText( i18n("Enter New Filename"),
				     i18n("A file with that name already exists. Please enter a new name:"),
				     newName, &ok, this, "renamedialog", validator );
    
  } while( ok && dir->find( newName ) );
  
  delete validator;

  return ok;
}


bool K3bDataUrlAddingDialog::checkForHiddenFiles( const QDir& dir )
{
  if( m_iAddHiddenFiles == 0 ) {
    // check for hidden files (QDir::Hidden does not include hidden directories :(
    QStringList hiddenFiles = dir.entryList( ".*", QDir::Hidden|QDir::All );
    hiddenFiles.remove(".");
    hiddenFiles.remove("..");
    if( hiddenFiles.count() > 0 ) {
      if( KMessageBox::questionYesNo( isVisible() ? this : parentWidget(),
				      i18n("Do you also want to add hidden files?"),
				      i18n("Hidden Files"), i18n("Add"), i18n("Do Not Add") ) == KMessageBox::Yes )
	m_iAddHiddenFiles = 1;
      else
	m_iAddHiddenFiles = -1;
    }
  }
  
  return ( m_iAddHiddenFiles == 1 );
}


bool K3bDataUrlAddingDialog::checkForSystemFiles( const QDir& dir )
{
  if( m_iAddSystemFiles == 0 ) {
    // QDir::System does include broken links
    QStringList systemFiles = dir.entryList( QDir::System );
    systemFiles.remove(".");
    systemFiles.remove("..");
    if( systemFiles.count() > 0 ) {
      if( KMessageBox::questionYesNo( isVisible() ? this : parentWidget(),
				      i18n("Do you also want to add system files "
					   "(FIFOs, sockets, device files, and broken symlinks)?"),
				      i18n("System Files"), i18n("Add"), i18n("Do Not Add") ) == KMessageBox::Yes )
	m_iAddSystemFiles = 1;
      else
	m_iAddSystemFiles = -1;
    }
  }
  
  return ( m_iAddSystemFiles == 1 );
}

#include "k3bdataurladdingdialog.moc"
