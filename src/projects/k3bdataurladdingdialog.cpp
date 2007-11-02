/*
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <k3bglobals.h>

#include "k3bdataurladdingdialog.h"
#include "k3bencodingconverter.h"

#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qdir.h>
#include <qfileinfo.h>

#include <k3bdatadoc.h>
#include <k3bdiritem.h>
#include <k3bcore.h>
#include <k3bfileitem.h>
#include <k3bmultichoicedialog.h>
#include <k3bvalidators.h>
#include <k3bglobals.h>
#include <k3bisooptions.h>
#include <k3b.h>
#include <k3bapplication.h>
#include <k3biso9660.h>
#include <k3bdirsizejob.h>
#include <k3binteractiondialog.h>
#include <k3bthread.h>
#include <k3bsignalwaiter.h>
#include <k3bexternalbinmanager.h>

#include <klocale.h>
#include <kurl.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kstdguiitem.h>
#include <kconfig.h>
#include <ksqueezedtextlabel.h>
#include <kprogress.h>

#include <unistd.h>


K3bDataUrlAddingDialog::K3bDataUrlAddingDialog( K3bDataDoc* doc, QWidget* parent, const char* name )
  : KDialogBase( Plain,
		 i18n("Adding files to project '%1'").arg(doc->URL().fileName()),
		 Cancel,
		 Cancel,
		 parent,
		 name,
		 true,
		 true ),
    m_bExistingItemsReplaceAll(false),
    m_bExistingItemsIgnoreAll(false),
    m_bFolderLinksFollowAll(false),
    m_bFolderLinksAddAll(false),
    m_iAddHiddenFiles(0),
    m_iAddSystemFiles(0),
    m_bCanceled(false),
    m_totalFiles(0),
    m_filesHandled(0),
    m_lastProgress(0)
{
  m_encodingConverter = new K3bEncodingConverter();

  QWidget* page = plainPage();
  QGridLayout* grid = new QGridLayout( page );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  m_counterLabel = new QLabel( page );
  m_infoLabel = new KSqueezedTextLabel( i18n("Adding files to project '%1'")
					.arg(doc->URL().fileName()) + "...", page );
  m_progressWidget = new KProgress( 0, page );

  grid->addWidget( m_counterLabel, 0, 1 );
  grid->addWidget( m_infoLabel, 0, 0 );
  grid->addMultiCellWidget( m_progressWidget, 1, 1, 0, 1 );

  m_dirSizeJob = new K3bDirSizeJob( this );
  connect( m_dirSizeJob, SIGNAL(finished(bool)),
	   this, SLOT(slotDirSizeDone(bool)) );

  // try to start with a reasonable size
  resize( (int)( fontMetrics().width( caption() ) * 1.5 ), sizeHint().height() );
}


K3bDataUrlAddingDialog::~K3bDataUrlAddingDialog()
{
  delete m_encodingConverter;
}


int K3bDataUrlAddingDialog::addUrls( const KURL::List& urls,
				     K3bDirItem* dir,
				     QWidget* parent )
{
  if( urls.isEmpty() )
    return 0;

  //
  // A common mistake by beginners is to try to burn an iso image
  // with a data project. Let's warn them
  //
  if( urls.count() == 1 ) {
    K3bIso9660 isoF( urls.first().path() );
    if( isoF.open() ) {
     if( KMessageBox::warningYesNo( parent,
				    i18n("<p>The file you are about to add to the project is an ISO9660 image. As such "
					 "it can be burned to a medium directly since it already contains a file "
					 "system.<br>"
					 "Are you sure you want to add this file to the project?"),
				    i18n("Adding image file to project"),
				    i18n("Add the file to the project"),
				    i18n("Burn the image directly") ) == KMessageBox::No ) {
       // very rough dvd image size test
       if( K3b::filesize( urls.first() ) > 1000*1024*1024 )
	 k3bappcore->k3bMainWindow()->slotWriteDvdIsoImage( urls.first() );
       else
	 k3bappcore->k3bMainWindow()->slotWriteCdImage( urls.first() );
       return 0;
     }
    }
  }

  K3bDataUrlAddingDialog dlg( dir->doc(), parent );
  dlg.m_urls = urls;
  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it )
    dlg.m_urlQueue.append( qMakePair( K3b::convertToLocalUrl(*it), dir ) );

  dlg.slotAddUrls();
  int ret = QDialog::Accepted;
  if( !dlg.m_urlQueue.isEmpty() ) {
    dlg.m_dirSizeJob->setUrls( urls );
    dlg.m_dirSizeJob->setFollowSymlinks( dir->doc()->isoOptions().followSymbolicLinks() );
    dlg.m_dirSizeJob->start();
    ret = dlg.exec();
  }

  // make sure the dir size job is finished
  dlg.m_dirSizeJob->cancel();
  K3bSignalWaiter::waitForJob( dlg.m_dirSizeJob );

  QString message = dlg.resultMessage();
  if( !message.isEmpty() )
    KMessageBox::detailedSorry( parent, i18n("Problems while adding files to the project."), message );

  return ret;
}


QString K3bDataUrlAddingDialog::resultMessage() const
{
  QString message;
  if( !m_unreadableFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("Insufficient permissions to read the following files") )
      .arg( m_unreadableFiles.join( "<br>" ) );
  if( !m_notFoundFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("Unable to find the following files") )
      .arg( m_notFoundFiles.join( "<br>" ) );
  if( !m_nonLocalFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("No non-local files supported") )
      .arg( m_unreadableFiles.join( "<br>" ) );
  if( !m_tooBigFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("It is not possible to add files bigger than %1").arg(KIO::convertSize(0xFFFFFFFF)) )
      .arg( m_tooBigFiles.join( "<br>" ) );
  if( !m_mkisofsLimitationRenamedFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("Some filenames had to be modified due to limitations in mkisofs") )
      .arg( m_mkisofsLimitationRenamedFiles.join( "<br>" ) );
  if( !m_invalidFilenameEncodingFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("The following filenames have an invalid encoding. You may fix this "
		 "with the convmv tool") )
      .arg( m_invalidFilenameEncodingFiles.join( "<br>" ) );

  return message;
}


int K3bDataUrlAddingDialog::moveItems( const QValueList<K3bDataItem*>& items,
				       K3bDirItem* dir,
				       QWidget* parent )
{
  return copyMoveItems( items, dir, parent, false );
}


int K3bDataUrlAddingDialog::copyItems( const QValueList<K3bDataItem*>& items,
				       K3bDirItem* dir,
				       QWidget* parent )
{
  return copyMoveItems( items, dir, parent, true );
}


int K3bDataUrlAddingDialog::copyMoveItems( const QValueList<K3bDataItem*>& items,
					   K3bDirItem* dir,
					   QWidget* parent,
					   bool copy )
{
  if( items.isEmpty() )
    return 0;

  K3bDataUrlAddingDialog dlg( dir->doc(), parent );
  dlg.m_infoLabel->setText( i18n("Moving files to project \"%1\"...").arg(dir->doc()->URL().fileName()) );
  dlg.m_copyItems = copy;

  for( QValueList<K3bDataItem*>::const_iterator it = items.begin(); it != items.end(); ++it ) {
    dlg.m_items.append( qMakePair( *it, dir ) );
    ++dlg.m_totalFiles;
    if( (*it)->isDir() ) {
      dlg.m_totalFiles += static_cast<K3bDirItem*>( *it )->numFiles();
      dlg.m_totalFiles += static_cast<K3bDirItem*>( *it )->numDirs();
    }
  }

  dlg.slotCopyMoveItems();
  int ret = QDialog::Accepted;
  if( !dlg.m_items.isEmpty() ) {
    dlg.m_progressWidget->setTotalSteps( dlg.m_totalFiles );
    ret = dlg.exec();
  }

  return ret;
}


void K3bDataUrlAddingDialog::slotCancel()
{
  m_bCanceled = true;
  m_dirSizeJob->cancel();
  KDialogBase::slotCancel();
}


void K3bDataUrlAddingDialog::slotAddUrls()
{
  if( m_bCanceled )
    return;

  // add next url
  KURL url = m_urlQueue.first().first;
  K3bDirItem* dir = m_urlQueue.first().second;
  m_urlQueue.remove( m_urlQueue.begin() );
  //
  // HINT:
  // we only use QFileInfo::absFilePath() and QFileInfo::isHidden()
  // both do not cause QFileInfo to stat, thus no speed improvement
  // can come from removing QFileInfo usage here.
  //
  QFileInfo info(url.path());
  QString absFilePath( info.absFilePath() );
  QString resolved( absFilePath );

  bool valid = true;
  k3b_struct_stat statBuf, resolvedStatBuf;
  bool isSymLink = false;
  bool isDir = false;
  bool isFile = false;

  ++m_filesHandled;

#if 0
  m_infoLabel->setText( url.path() );
  if( m_totalFiles == 0 )
    m_counterLabel->setText( QString("(%1)").arg(m_filesHandled) );
  else
    m_counterLabel->setText( QString("(%1/%2)").arg(m_filesHandled).arg(m_totalFiles) );
#endif

  //
  // 1. Check if we want and can add the url
  //

  if( !url.isLocalFile() ) {
    valid = false;
    m_nonLocalFiles.append( url.path() );
  }

  else if( k3b_lstat( QFile::encodeName(absFilePath), &statBuf ) != 0 ) {
    valid = false;
    m_notFoundFiles.append( url.path() );
  }

  else if( !m_encodingConverter->encodedLocally( QFile::encodeName( url.path() ) ) ) {
    valid = false;
    m_invalidFilenameEncodingFiles.append( url.path() );
  }

  else {
    isSymLink = S_ISLNK(statBuf.st_mode);
    isFile = S_ISREG(statBuf.st_mode);
    isDir = S_ISDIR(statBuf.st_mode);

    // symlinks are always readable and can always be added to a project
    // but we need to know if the symlink points to a directory
    if( isSymLink ) {
      resolved = K3b::resolveLink( absFilePath );
      k3b_stat( QFile::encodeName(resolved), &resolvedStatBuf );
      isDir = S_ISDIR(resolvedStatBuf.st_mode);
    }

    else {
      if( ::access( QFile::encodeName( absFilePath ), R_OK ) != 0 ) {
	valid = false;
	m_unreadableFiles.append( url.path() );
      }
      else if( isFile && (unsigned long long)statBuf.st_size >= 0xFFFFFFFFULL ) {
          if ( !k3bcore->externalBinManager()->binObject( "mkisofs" )->hasFeature( "no-4gb-limit" ) ) {
              valid = false;
              m_tooBigFiles.append( url.path() );
          }
      }
    }

    // FIXME: if we do not add hidden dirs the progress gets messed up!

    //
    // check for hidden and system files
    //
    if( valid ) {
      if( info.isHidden() && !addHiddenFiles() )
	valid = false;
      if( S_ISCHR(statBuf.st_mode) ||
	  S_ISBLK(statBuf.st_mode) ||
	  S_ISFIFO(statBuf.st_mode) ||
	  S_ISSOCK(statBuf.st_mode) )
	if( !addSystemFiles() )
	  valid = false;
      if( isSymLink )
	if( S_ISCHR(resolvedStatBuf.st_mode) ||
	    S_ISBLK(resolvedStatBuf.st_mode) ||
	    S_ISFIFO(resolvedStatBuf.st_mode) ||
	    S_ISSOCK(resolvedStatBuf.st_mode) )
	  if( !addSystemFiles() )
	    valid = false;
    }
  }


  //
  // 2. Handle the url
  //

  QString newName = url.fileName();

  // filenames cannot end in backslashes (mkisofs problem. See comments in k3bisoimager.cpp (escapeGraftPoint()))
  bool bsAtEnd = false;
  while( newName[newName.length()-1] == '\\' ) {
    newName.truncate( newName.length()-1 );
    bsAtEnd = true;
  }
  if( bsAtEnd )
    m_mkisofsLimitationRenamedFiles.append( url.path() + " -> " + newName );

  // backup dummy name
  if( newName.isEmpty() )
    newName = "1";

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
      if( oldItem->isDir() && isDir )
	newDirItem = dynamic_cast<K3bDirItem*>(oldItem);

      //
      // we cannot replace files in the old session with dirs and vice versa (I think)
      // files are handled in K3bFileItem constructor and dirs handled above
      //
      else if( oldItem->isFromOldSession() &&
	       isDir != oldItem->isDir() ) {
	if( !getNewName( newName, dir, newName ) )
	  valid = false;
      }

      else if( m_bExistingItemsIgnoreAll )
	valid = false;

      else if( oldItem->localPath() == resolved ) {
	//
	// Just ignore if the same file is added again
	//
	valid = false;
      }

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
					      .arg('/' + dir->k3bPath()),
					      QMessageBox::Warning,
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
    // let's see if this link starts a loop
    // that means if it points to some folder above this one
    // if so we cannot follow it anyway
    if( isDir && isSymLink && !absFilePath.startsWith( resolved ) ) {
      bool followLink = dir->doc()->isoOptions().followSymbolicLinks() || m_bFolderLinksFollowAll;
      if( !followLink && !m_bFolderLinksAddAll ) {
	switch( K3bMultiChoiceDialog::choose( i18n("Adding link to folder"),
					      i18n("<p>'%1' is a symbolic link to folder '%2'."
						   "<p>If you intend to make K3b follow symbolic links you should consider letting K3b do this now "
						   "since K3b will not be able to do so afterwards because symbolic links to folders inside a "
						   "K3b project cannot be resolved."
						   "<p><b>If you do not intend to enable the option <em>follow symbolic links</em> you may safely "
						   "ignore this warning and choose to add the link to the project.</b>")
					      .arg(absFilePath)
					      .arg(resolved ),
					      QMessageBox::Warning,
					      this,
					      0,
					      5,
					      i18n("Follow link now"),
					      i18n("Always follow links"),
					      i18n("Add link to project"),
					      i18n("Always add links"),
					      KStdGuiItem::cancel() ) ) {
	case 2:
	  m_bFolderLinksFollowAll = true;
	case 1:
	  followLink = true;
	  break;
	case 4:
	  m_bFolderLinksAddAll = true;
	case 3:
	  followLink = false;
	  break;
	case 5:
	  slotCancel();
	  return;
	}
      }

      if( followLink ) {
	absFilePath = resolved;
	isSymLink = false;

	// count the files in the followed dir
	if( m_dirSizeJob->active() )
	  m_dirSizeQueue.append( KURL::fromPathOrURL(absFilePath) );
	else {
	  m_progressWidget->setTotalSteps( 0 );
	  m_dirSizeJob->setUrls( KURL::fromPathOrURL(absFilePath) );
	  m_dirSizeJob->start();
	}
      }
    }
  }


  //
  // Project valid also (we overwrite or renamed)
  // now create the new item
  //
  if( valid ) {
    //
    // Set the volume id from the first added url
    // only if the doc was not changed yet
    //
    if( m_urls.count() == 1 &&
	!dir->doc()->isModified() &&
	!dir->doc()->isSaved() ) {
      dir->doc()->setVolumeID( K3b::removeFilenameExtension( newName ) );
    }

    if( isDir && !isSymLink ) {
      if( !newDirItem ) { // maybe we reuse an already existing dir
	newDirItem = new K3bDirItem( newName , dir->doc(), dir );
	newDirItem->setLocalPath( url.path() ); // HACK: see k3bdiritem.h
      }

      QDir newDir( absFilePath );
      int dirFilter = QDir::All|QDir::Hidden|QDir::System;

      QStringList dlist = newDir.entryList( dirFilter );
      const QString& dot = KGlobal::staticQString( "." );
      const QString& dotdot = KGlobal::staticQString( ".." );
      dlist.remove( dot );
      dlist.remove( dotdot );

      for( QStringList::Iterator it = dlist.begin(); it != dlist.end(); ++it ) {
	m_urlQueue.append( qMakePair( KURL::fromPathOrURL(absFilePath + '/' + *it), newDirItem ) );
      }
    }
    else {
      (void)new K3bFileItem( &statBuf, &resolvedStatBuf, url.path(), dir->doc(), dir, newName );
    }
  }

  if( m_urlQueue.isEmpty() ) {
    m_dirSizeJob->cancel();
    m_progressWidget->setProgress( 100 );
    accept();
  }
  else {
    updateProgress();
    QTimer::singleShot( 0, this, SLOT(slotAddUrls()) );
  }
}


void K3bDataUrlAddingDialog::slotCopyMoveItems()
{
  if( m_bCanceled )
    return;

  //
  // Pop first item from the item list
  //
  K3bDataItem* item = m_items.first().first;
  K3bDirItem* dir = m_items.first().second;
  m_items.remove( m_items.begin() );

  ++m_filesHandled;
  m_infoLabel->setText( item->k3bPath() );
  if( m_totalFiles == 0 )
    m_counterLabel->setText( QString("(%1)").arg(m_filesHandled) );
  else
    m_counterLabel->setText( QString("(%1/%2)").arg(m_filesHandled).arg(m_totalFiles) );


  if( dir == item->parent() ) {
    kdDebug() << "(K3bDataUrlAddingDialog) trying to move an item into its own parent dir." << endl;
  }
  else if( dir == item ) {
    kdDebug() << "(K3bDataUrlAddingDialog) trying to move an item into itselft." << endl;
  }
  else {
    //
    // Let's see if an item with that name alredy exists
    //
    if( K3bDataItem* oldItem = dir->find( item->k3bName() ) ) {
      //
      // reuse an existing dir: move all child items into the old dir
      //
      if( oldItem->isDir() && item->isDir() ) {
	const QPtrList<K3bDataItem>& cl = dynamic_cast<K3bDirItem*>( item )->children();
	for( QPtrListIterator<K3bDataItem> it( cl ); *it; ++it )
	  m_items.append( qMakePair( *it, dynamic_cast<K3bDirItem*>( oldItem ) ) );

	// FIXME: we need to remove the old dir item
      }

      //
      // we cannot replace files in the old session with dirs and vice versa (I think)
      // files are handled in K3bFileItem constructor and dirs handled above
      //
      else if( oldItem->isFromOldSession() &&
	       item->isDir() != oldItem->isDir() ) {
	QString newName;
	if( getNewName( newName, dir, newName ) ) {
	  if( m_copyItems )
	    item = item->copy();
	  item->setK3bName( newName );
	  dir->addDataItem( item );
	}
      }

      else if( m_bExistingItemsReplaceAll ) {
	//
	// if we replace an item from an old session K3bDirItem::addDataItem takes care
	// of replacing the item
	//
	if( !oldItem->isFromOldSession() )
	  delete oldItem;
	if( m_copyItems )
	  item = item->copy();
	dir->addDataItem( item );
      }

      else if( !m_bExistingItemsIgnoreAll ) {
	switch( K3bMultiChoiceDialog::choose( i18n("File already exists"),
					      i18n("<p>File <em>%1</em> already exists in "
						   "project folder <em>%2</em>.")
					      .arg( item->k3bName() )
					      .arg("/" + dir->k3bPath()),
					      QMessageBox::Warning,
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
	  //
	  // if we replace an item from an old session K3bDirItem::addDataItem takes care
	  // of replacing the item
	  //
	  if( !oldItem->isFromOldSession() )
	    delete oldItem;
	  if( m_copyItems )
	    item = item->copy();
	  dir->addDataItem( item );
	  break;
	case 4: // ignore all
	  m_bExistingItemsIgnoreAll = true;
	  // fallthrough
	case 3: // ignore
	  // do nothing
	  break;
	case 5: {// rename
	  QString newName;
	  if( getNewName( newName, dir, newName ) ) {
	    if( m_copyItems )
	      item = item->copy();
	    item->setK3bName( newName );
	    dir->addDataItem( item );
	  }
	  break;
	}
	case 6: // cancel
	  slotCancel();
	  return;
	}
      }
    }

    //
    // No old item with the same name
    //
    else {
      if( m_copyItems )
	item = item->copy();
      dir->addDataItem( item );
    }
  }

  if( m_items.isEmpty() ) {
    m_dirSizeJob->cancel();
    accept();
  }
  else {
    updateProgress();
    QTimer::singleShot( 0, this, SLOT(slotCopyMoveItems()) );
  }
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


bool K3bDataUrlAddingDialog::addHiddenFiles()
{
  if( m_iAddHiddenFiles == 0 ) {
    // FIXME: the isVisible() stuff makes the static addUrls method not return (same below)
    if( KMessageBox::questionYesNo( /*isVisible() ? */this/* : parentWidget()*/,
				    i18n("Do you also want to add hidden files?"),
				    i18n("Hidden Files"), i18n("Add"), i18n("Do Not Add") ) == KMessageBox::Yes )
      m_iAddHiddenFiles = 1;
    else
      m_iAddHiddenFiles = -1;
  }

  return ( m_iAddHiddenFiles == 1 );
}


bool K3bDataUrlAddingDialog::addSystemFiles()
{
  if( m_iAddSystemFiles == 0 ) {
    if( KMessageBox::questionYesNo( /*isVisible() ? */this/* : parentWidget()*/,
				    i18n("Do you also want to add system files "
					 "(FIFOs, sockets, device files, and broken symlinks)?"),
				    i18n("System Files"), i18n("Add"), i18n("Do Not Add") ) == KMessageBox::Yes )
      m_iAddSystemFiles = 1;
    else
      m_iAddSystemFiles = -1;
  }

  return ( m_iAddSystemFiles == 1 );
}


void K3bDataUrlAddingDialog::slotDirSizeDone( bool success )
{
  if( success ) {
    m_totalFiles += m_dirSizeJob->totalFiles() + m_dirSizeJob->totalDirs();
    if( m_dirSizeQueue.isEmpty() ) {
      m_progressWidget->setTotalSteps( 100 );
      updateProgress();
    }
    else {
      m_dirSizeJob->setUrls( m_dirSizeQueue.back() );
      m_dirSizeQueue.pop_back();
      m_dirSizeJob->start();
    }
  }
}


void K3bDataUrlAddingDialog::updateProgress()
{
  if( m_totalFiles > 0 ) {
    unsigned int p = 100*m_filesHandled/m_totalFiles;
    if( p > m_lastProgress ) {
      m_lastProgress = p;
      m_progressWidget->setProgress( p );
    }
  }
  else {
    // make sure the progress bar shows something
    m_progressWidget->setProgress( m_filesHandled );
  }
}

#include "k3bdataurladdingdialog.moc"
